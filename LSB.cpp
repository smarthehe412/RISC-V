#include "parser.cpp"

class LoadStoreBuffer
{
    const static int N=100;
    enum state {wait,calc,done};
public:
    struct Buffer
    {
        struct node
        {
            bool busy;
            int dest,op,val,id;
            int st;
            node(){busy=false;dest=op=val=id=0;st=wait;}
            node(int dest,int op,int val,int id): dest(dest),op(op),val(val),id(id) {busy=true;st=wait;}
        } que[N];
        int h,t;
        Buffer() {h=t=0;}
        bool full(){return h==t&&que[h].busy==true;}
        bool empty(){return h==t&&que[h].busy==false;}
        void nextState(int pos){que[pos].st++;}
        bool isDone(int pos){return que[pos].st==done;}
        int getpos(){return h;}
        int nextpos(int now){return (now+1)%N;}
        int getdir(int pos){return que[pos].dest;}
        int getop(int pos){return que[pos].op;}
        int getv(int pos){return que[pos].val;}
        int getid(int pos){return que[pos].id;}
        int findNext(int pos)
        {
            if((h+1)%N!=t) return (h+1)%N;
            else return -1;
        }
        void pop()
        {
            que[h]=node();
            h=(h+1)%N;
        }
        void push(int dest,int op,int val,int id)
        {
            que[t]=node(dest,op,val,id);
            t=(t+1)%N;
        }
    } now,next;
    void nextClock(){now=next;}
    void prepare(int pos,int va)
    {
        next.que[pos].val=va;
        next.que[pos].st=done;
    }
} LSB;
class MemoryReadWriter
{
    int rem,op,pos,dir,val;
    int rem2,op2,pos2,dir2,val2;
public:
    MemoryReadWriter(){rem=op=pos=dir=val=rem2=op2=pos2=dir2=val2=0;}
    void nextClock(){rem=rem2,op=op2,pos=pos2,dir=dir2,val=val2;}
    bool busy(){return rem>0;}
    void calc()
    {
        if(busy())
        {
            prompt();
            return;
        }
        if(LSB.now.empty()) return;
        while(pos2!=LSB.now.t&&LSB.now.isDone(pos2)) pos2=LSB.now.nextpos(pos2);
        if(pos2==LSB.now.t) return;
        rem2=3;op2=LSB.now.getop(pos2);dir2=LSB.now.getdir(pos2);val2=LSB.now.getv(pos2);
        LSB.next.nextState(pos2);
        prompt2();
    }
    void prompt()
    {
        if(!rem) return;
        rem2=rem-1;
        if(!rem2)
        {
            dir%=MEM_MAX;
            if(op>0) for(int i=0;i<op;i++) mem[dir+i]=val>>(i*8)&255;
            else val2=getValxx(mem,dir,dir-op-1);
            LSB.prepare(pos,val2);
        }
    }
    void prompt2()
    {
        if(!rem2) return;
        rem2--;
        if(!rem2)
        {
            dir%=MEM_MAX;
            if(op>0) for(int i=0;i<op;i++) mem[dir+i]=val>>(i*8)&255;
            else val2=getValxx(mem,dir,dir-op-1);
            LSB.prepare(pos,val2);
        }
    }
} mrw;
