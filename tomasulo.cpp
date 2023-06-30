#include "command.cpp"
#include "ALU.cpp"
#include "LSB.cpp"
bool end;
int reg[32],rely[32],CLEAR;
int reg2[32],rely2[32];
struct information
{
    int dest,vj,vk,qj,qk,op,oth;
    information(){dest=vj=vk=qj=qk=oth=0;}
    information(int dest,int vj,int vk,int qj,int qk,int op): dest(dest),vj(vj),vk(vk),qj(qj),qk(qk),op(op) {oth=0;}
    void print()
    {
        fprintf(stderr,"dest:%d vj:%d vk:%d qj:%d qk:%d op:%d\n",dest,vj,vk,qj,qk,op);
    }
};
information decode(int pc)
{
    int dest=getDest(pc),vj=0,vk=0,qj=-1,qk=-1,op=getOp(pc);
    char typ=getType1(pc);
    if(typ=='R'||typ=='B')
    {
        qj=getRs1(pc),qk=getRs2(pc);
        if(rely[qj]!=-1)
        {
            if(rely2[qj]==-1) vj=reg2[qj],qj=-1;
            else qj=rely[qj];
        }
        else vj=reg[qj],qj=-1;
        if(rely[qk]!=-1)
        {
            if(rely2[qk]==-1) vk=reg2[qk],qk=-1;
            else qk=rely[qk];
        }
        else vk=reg[qk],qk=-1;
    }
    else if(typ=='I'||typ=='S')
    {
        qj=getRs1(pc);
        if(rely[qj]!=-1)
        {
            if(rely2[qj]==-1) vj=reg2[qj],qj=-1;
            else qj=rely[qj];
        }
        else vj=reg[qj],qj=-1;
        vk=getImm(pc);
    }
    else if(typ=='U'||typ=='J') vj=getImm(pc);
    information ret(dest,vj,vk,qj,qk,op);
    if(typ=='B') ret.oth=bP.getPredict(pc);
    return ret;
}
class ReorderBuffer
{
    const static int N=500;
    enum state {execute,ready,commit};
public:
    struct Buffer
    {
        struct node
        {
            bool busy,wait;
            int dest,val;
            int st,pc,oth;
            node(){busy=wait=false;dest=val=pc=oth=0;st=execute;}
            node(int pc,int dest,int oth): pc(pc),dest(dest),oth(oth) {val=0,st=execute,busy=true,wait=false;}
        } que[N];
        int h,t;
        Buffer(){h=t=0;}
        bool full(){return h==t&&que[h].busy;}
        void clear()
        {
            if(full()) for(int i=0;i<N;i++) que[i]=node();
            else
            {
                for(int i=h;i!=t;i++)
                {
                    if(i==N) i=0;
                    que[i]=node(); 
                }
            } 
        }
        bool isExecute(){return que[h].st==execute;}
        bool isReady(){return que[h].st==ready;}
        bool isCommit(){return que[h].st==commit;}
        bool isWait(){return que[h].wait;}
        void letWait(){que[h].wait=true;}
        void push(int pc)
        {
            information inf=decode(pc);
            que[t]=node(pc,inf.dest,inf.oth);
            int typ=getType2(pc);
            if(inf.op==none) que[t].val=inf.vj,que[t].st=ready;
            else
            {
                if(typ==LB||typ==LH||typ==LW||typ==LBU||typ==LHU) RS.next.ins(0,inf.vj,inf.vk,inf.qj,inf.qk,inf.op,t);
                else RS.next.ins(inf.dest,inf.vj,inf.vk,inf.qj,inf.qk,inf.op,t);
            }
            if(inf.dest) rely2[inf.dest]=t;
            t=(t+1)%N;
        }
        int toppc(){return que[h].pc;}
        int topid(){return h;}
        int topv(){return que[h].val;}
        int topd(){return que[h].dest;}
        int topoth(){return que[h].oth;}
        void pop()
        {
            if(que[h].dest&&rely[que[h].dest]==h&&rely2[que[h].dest]==h) rely2[que[h].dest]=-1;
            //printf("%d\n",que[h].pc);
            //for(int i=0;i<32;i++) printf("%d ",reg[i]);printf("\n");
            que[h]=node();
            h=(h+1)%N;
        }
    } now,next;
    void nextClock(){now=next;}
    void fetchALU()
    {
        int tmp=-1;
        while((tmp=RS.now.findDone(tmp+1))!=-1)
        {
            int res=RS.now.getret(tmp),pos=RS.now.getid(tmp);
            //if(res==4952) fprintf(stderr,"pc:%d ",now.que[pos].pc),decode(now.que[pos].pc).print();
            RS.next.del(tmp);
            next.que[pos].val=res;
            next.que[pos].st++;
        }
    }
    void fetchLSB()
    {
        int tmp=LSB.now.getpos();
        while(LSB.now.isDone(tmp))
        {
            int res=LSB.now.getv(tmp),pos=LSB.now.getid(tmp);
            LSB.next.pop();
            next.que[pos].val=res;
            next.que[pos].st++;
            next.que[pos].wait=false;
            tmp=LSB.now.nextpos(tmp);
        }
    }
} RoB;
void branchClear()
{
    CLEAR=1;
    for(int i=0;i<32;i++) rely2[i]=-1;
    RoB.next.clear();
    RS.next.clear();
    alu.clear();
}
void fetch()
{
    if(CLEAR||RoB.now.full()) return;
    int pc=cQ.getCommand();
    if(pc==-1) return;
    RoB.next.push(pc);
}
void execute()
{
    if(CLEAR) {mrw.calc();return;}
    alu.calc();
    mrw.calc();
    RoB.fetchALU();
    RoB.fetchLSB();
}
void commit()
{
    if(RoB.now.isExecute()||RoB.now.isWait()) return;
    int pc=RoB.now.toppc(),dest=RoB.now.topd(),val=RoB.now.topv();
    //if(pc==4260) system("pause");
    //printf("pc=%d val=%d ",pc,val),decode(pc).print();
    if(getValxx(mem,pc,pc+3)==0x0ff00513) {end=true;return;}
    int typ=getType2(pc);
    switch(typ)
    {
        case LUI:if(dest) reg2[dest]=val,RS.broadcast(RoB.now.topid(),reg2[dest]);RoB.next.pop();break;
        case AUIPC:if(dest) reg2[dest]=cQ.getpc()+val,RS.broadcast(RoB.now.topid(),reg2[dest]);RoB.next.pop();break;
        case JAL:
        {
            if(dest) reg2[dest]=pc+4;
            cQ.modify(pc+val);
            branchClear();
            break;
        }
        case JALR:
        {
            if(dest) reg2[dest]=pc+4;
            cQ.modify(val);
            branchClear();
            break;
        }
        case BEQ:
        case BNE:
        case BLT:
        case BGE:
        case BLTU:
        case BGEU:
        {
            int is=RoB.now.topoth();
            if(is!=val)
            {
                if(val) cQ.modify(pc+getImm(pc));
                else cQ.modify(pc+4);
                bP.changeFreq(false,pc);
                branchClear();
            }
            else bP.changeFreq(true,pc),RoB.next.pop();
            break;
        }
        case LB:
        {
            if(RoB.now.isReady()) LSB.next.push(val,-1,0,RoB.now.topid()),RoB.next.letWait();
            else
            {
                if(dest) reg2[dest]=sext(val,8),RS.broadcast(RoB.now.topid(),reg2[dest]);
                RoB.next.pop();
            }
            break;
        }
        case LH:
        {
            if(RoB.now.isReady()) LSB.next.push(val,-2,0,RoB.now.topid()),RoB.next.letWait();
            else
            {
                if(dest) reg2[dest]=sext(val,16),RS.broadcast(RoB.now.topid(),reg2[dest]);
                RoB.next.pop();
            }
            break;
        }
        case LW:
        {
            if(RoB.now.isReady()) LSB.next.push(val,-4,0,RoB.now.topid()),RoB.next.letWait();
            else
            {
                if(dest) reg2[dest]=val,RS.broadcast(RoB.now.topid(),reg2[dest]);
                RoB.next.pop();
            }
            break;
        }
        case LBU:
        {
            if(RoB.now.isReady()) LSB.next.push(val,-1,0,RoB.now.topid()),RoB.next.letWait();
            else
            {
                if(dest) reg2[dest]=val,RS.broadcast(RoB.now.topid(),reg2[dest]);
                RoB.next.pop();
            }
            break;
        }
        case LHU:
        {
            if(RoB.now.isReady()) LSB.next.push(val,-2,0,RoB.now.topid()),RoB.next.letWait();
            else
            {
                if(dest) reg2[dest]=val,RS.broadcast(RoB.now.topid(),reg2[dest]);
                RoB.next.pop();
            }
            break;
        }
        case SB:
        {
            if(RoB.now.isReady()) LSB.next.push(val,1,reg[getRs2(pc)]&255,RoB.now.topid()),RoB.next.letWait();
            else RoB.next.pop();
            break;
        }
        case SH:
        {
            if(RoB.now.isReady()) LSB.next.push(val,2,reg[getRs2(pc)]&65535,RoB.now.topid()),RoB.next.letWait();
            else RoB.next.pop();
            break;
        }
        case SW:
        {
            if(RoB.now.isReady()) LSB.next.push(val,4,reg[getRs2(pc)],RoB.now.topid()),RoB.next.letWait();
            else RoB.next.pop();
            break;
        }
        case ADDI:
        case SLTI:
        case SLTIU:
        case XORI:
        case ORI:
        case ANDI:
        case SLLI:
        case SRLI:
        case SRAI:
        case ADD:
        case SUB:
        case SLL:
        case SLT:
        case SLTU:
        case XOR:
        case SRL:
        case SRA:
        case OR:
        case AND:if(dest) reg2[dest]=val,RS.broadcast(RoB.now.topid(),reg2[dest]);RoB.next.pop();break;
    }
}
void nextClock()
{
    CLEAR=0;
    for(int i=0;i<32;i++) reg[i]=reg2[i],rely[i]=rely2[i];
    RoB.nextClock();
    RS.nextClock();
    alu.nextClock();
    LSB.nextClock();
    mrw.nextClock();
    cQ.nextClock();
}
int main()
{
    //freopen("task.data","r",stdin);
    //freopen("tomasulo.out","w",stdout);
    memset(rely,-1,sizeof(rely));
    memset(rely2,-1,sizeof(rely2));
    int cnt=0;
    getAllCommands();
    while(1)
    {
        fetch();
        execute();
        commit();
        nextClock();
        if(end)
        {
            printf("%u",((unsigned int)reg[10])&255u);
            return 0;
        }
        //printf("%u\n",reg[10]);
    }
    return 0;
}