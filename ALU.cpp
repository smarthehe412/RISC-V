#include "parser.cpp"
class ReservationStationALU
{
    const static int N=100;
    enum state {wait,calc,done};
public:
    struct ReservationStation
    {
        int bid[5],bva[5];
        struct node
        {
            bool busy;
            int dest,vj,vk,qj,qk,op,id,ret;
            int st;
            node(){busy=false;dest=vj=vk=op=ret=id=0;qj=qk=-1;st=wait;}
            node(int dest,int vj,int vk,int qj,int qk,int op,int id): dest(dest),vj(vj),vk(vk),qj(qj),qk(qk),op(op),id(id) {busy=true;st=wait;ret=0;}
        } rs[N];
        ReservationStation()
        {
            for(int i=0;i<5;i++) bid[i]=-2,bva[i]=0;
        }
        void clear()
        {
            for(int i=0;i<N;i++) rs[i]=node();
            for(int i=0;i<5;i++) bid[i]=-2,bva[i]=0;
        }
        void ins(int dest,int vj,int vk,int qj,int qk,int op,int id)
        {
            for(int i=0;i<N;i++)
            {
                if(!rs[i].busy)
                {
                    rs[i]=node(dest,vj,vk,qj,qk,op,id);
                    for(int j=0;j<5;j++) if(rs[i].qj==bid[j]) rs[i].qj=-1,rs[i].vj=bva[j];
                    for(int j=0;j<5;j++) if(rs[i].qk==bid[j]) rs[i].qk=-1,rs[i].vk=bva[j];
                    return;
                }
            }
        }
        void del(int pos) {rs[pos]=node();}
        int findReady()
        {
            for(int i=0;i<N;i++)
            {
                if(!rs[i].busy||rs[i].st!=wait) continue;
                if(rs[i].qj==-1&&rs[i].qk==-1) return i; 
            }
            return -1;
        }
        int findDone(int st)
        {
            for(int i=st;i<N;i++)
            {
                if(!rs[i].busy) continue;
                if(rs[i].st==done) return i;
            }
            return -1;
        }
        int getx(int pos){return rs[pos].vj;}
        int gety(int pos){return rs[pos].vk;}
        int getop(int pos){return rs[pos].op;}
        int getid(int pos){return rs[pos].id;}
        int getret(int pos){return rs[pos].ret;}
        void nextState(int pos){rs[pos].st++;}
    } now,next;
    void nextClock()
    {
        now=next;
        for(int i=0;i<5;i++) next.bid[i]=-2,next.bva[i]=0;
    }
    void broadcast(int id,int va)
    {
        for(int i=0;i<5;i++)
        {
            if(next.bid[i]!=-2) continue;
            next.bid[i]=id,next.bva[i]=va;
            break;
        }
        for(int i=0;i<N;i++)
        {
            if(next.rs[i].qj==id) next.rs[i].qj=-1,next.rs[i].vj=va;
            if(next.rs[i].qk==id) next.rs[i].qk=-1,next.rs[i].vk=va;
        }
    }
    void prepare(int pos,int va)
    {
        next.rs[pos].ret=va;
        next.rs[pos].st=done;
        if(next.rs[pos].dest) broadcast(next.rs[pos].id,va);
    }
} RS;
class ALU
{
    int rem,res,pos;
    int rem2,res2,pos2;
public:
    ALU(){rem=res=rem2=res2=0;pos=pos2=-1;}
    void clear() {rem2=res2=0;pos2=-1;}
    void nextClock(){rem=rem2,res=res2,pos=pos2;}
    bool busy(){return rem>0;}
    void calc()
    {
        if(busy())
        {
            prompt();
            return;
        }
        int tmp=RS.now.findReady();
        if(tmp==-1) return;
        int x=RS.now.getx(tmp),y=RS.now.gety(tmp),op=RS.now.getop(tmp);
        RS.next.nextState(tmp);
        rem2=1;pos2=tmp;
        switch(op)
        {
            case eq:res2=(x==y);break;
            case neq:res2=(x!=y);break;
            case le:res2=(x<y);break;
            case geq:res2=(x>=y);break;
            case leu:
            {
                unsigned int tx=x,ty=y;
                res2=(tx<ty);break;
            }
            case gequ:
            {
                unsigned int tx=x,ty=y;
                res2=(tx>=ty);break;
            }
            case add:res2=(x+y);break;
            case xori:res2=(x^y);break;
            case ori:res2=(x|y);break;
            case andi:res2=(x&y);break;
            case slli:res2=(x<<(y&31));break;
            case srli:
            {
                unsigned int tx=x;
                res2=(tx>>(y&31));break;
            }
            case srai:res2=(x>>(y&31));break;
            case sub:res2=(x-y);break;
        }
        prompt2();
    }
    void prompt()
    {
        if(!rem) return;
        rem2=rem-1;
        if(!rem2) RS.prepare(pos,res);
    }
    void prompt2()
    {
        if(!rem2) return;
        rem2--;
        if(!rem2) RS.prepare(pos2,res2);
    }
} alu;