#include <iostream>
#include <cstring>
const int MEM_MAX=1e6;
static unsigned char mem[MEM_MAX];
enum commands {LUI,AUIPC,JAL,JALR,BEQ,BNE,BLT,BGE,BLTU,BGEU,LB,LH,LW,LBU,LHU,SB,SH,SW,ADDI,SLTI,SLTIU,XORI,ORI,ANDI,SLLI,SRLI,SRAI,ADD,SUB,SLL,SLT,
SLTU,XOR,SRL,SRA,OR,AND};
enum operators {none=-1,eq,neq,le,geq,leu,gequ,add,xori,ori,andi,slli,srli,srai,sub,mul,dv};
const int ops[37]={none,none,none,add,eq,neq,le,geq,leu,gequ,add,add,add,add,add,add,add,add,add,le,leu,xori,ori,andi,slli,srli,srai,add,sub,slli,
le,leu,xori,srli,srai,ori,andi};
unsigned int getValxx(const unsigned char *s,int l,int r)
{
    unsigned int ret=0;
    for(int i=l;i<=r;i++) ret|=s[i]<<((i-l)*8);
    return ret; 
}
unsigned int getPart(unsigned int v,int l,int r)
{
    return (v>>r)&((1u<<(l-r+1))-1);
}
int sext(unsigned int num,int len)
{
    if(num&(1u<<(len-1))) return num|((-1u)>>len<<len);
    else return num;
}
unsigned int getValx(const char *s,int l,int r)
{
    unsigned int ret=0;
    for(int i=l;i<=r;i++)
    {
        int x=('0'<=s[i]&&s[i]<='9')?s[i]-'0':s[i]-'A'+10;
        ret=ret<<4|x;
    }
    return ret; 
}
char getType1(int l)
{
    int s=getPart(getValxx(mem,l,l+3),6,0);
    if(s==0b0110111||s==0b0010111) return 'U';
    if(s==0b1101111) return 'J';
    if(s==0b1100011) return 'B';
    if(s==0b1100111||s==0b0000011||s==0b0010011) return 'I';
    if(s==0b0100011) return 'S';
    if(s==0b0110011) return 'R';
    return '?';
}
int getType2(int l)
{
    unsigned int in=getValxx(mem,l,l+3);
    int s=getPart(in,6,0);
    if(s==0b0110111) return LUI;
    if(s==0b0010111) return AUIPC;
    if(s==0b1101111) return JAL;
    if(s==0b1100111) return JALR;
    if(s==0b1100011)
    {
        int t=getPart(in,14,12);
        if(t==0b000) return BEQ;
        if(t==0b001) return BNE;
        if(t==0b100) return BLT;
        if(t==0b101) return BGE;
        if(t==0b110) return BLTU;
        if(t==0b111) return BGEU;
    }
    if(s==0b0000011)
    {
        int t=getPart(in,14,12);
        if(t==0b000) return LB;
        if(t==0b001) return LH;
        if(t==0b010) return LW;
        if(t==0b100) return LBU;
        if(t==0b101) return LHU;
    }
    if(s==0b0100011)
    {
        int t=getPart(in,14,12);
        if(t==0b000) return SB;
        if(t==0b001) return SH;
        if(t==0b010) return SW;
    }
    if(s==0b0010011)
    {
        int t=getPart(in,14,12);
        if(t==0b000) return ADDI;
        if(t==0b010) return SLTI;
        if(t==0b011) return SLTIU;
        if(t==0b100) return XORI;
        if(t==0b110) return ORI;
        if(t==0b111) return ANDI;
        if(t==0b001) return SLLI;
        if(t==0b101)
        {
            if(getPart(in,30,30)==0) return SRLI;
            else return SRAI;
        }
    }
    if(s==0b0110011)
    {
        int t=getPart(in,14,12);
        if(t==0b000)
        {
            if(getPart(in,30,30)==0) return ADD;
            else return SUB;
        }
        if(t==0b001) return SLL;
        if(t==0b010) return SLT;
        if(t==0b011) return SLTU;
        if(t==0b100) return XOR;
        if(t==0b101)
        {
            if(getPart(in,30,30)==0) return SRL;
            else return SRA;
        }
        if(t==0b110) return OR;
        if(t==0b111) return AND;
    }
    return -1;
}
int getOp(int l)
{
    int type=getType2(l);
    return ops[type];
}
int getImm(int l)
{
    int in=getValxx(mem,l,l+3);
    char typ=getType1(l);
    if(typ=='I') return sext(getPart(in,31,20),12);
    if(typ=='S') return sext(getPart(in,31,25)<<5|getPart(in,11,7),12);
    if(typ=='B') return sext(getPart(in,31,31)<<12|getPart(in,30,25)<<5|getPart(in,11,8)<<1|getPart(in,7,7)<<11,13);
    if(typ=='U') return sext(getPart(in,31,12)<<12,32);
    if(typ=='J') return sext(getPart(in,31,31)<<20|getPart(in,30,21)<<1|getPart(in,20,20)<<11|getPart(in,19,12)<<12,21);
    return 114514;
}
int getRd(int l)
{
    return getPart(getValxx(mem,l,l+3),11,7);
}
int getRs1(int l)
{
    if(getType1(l)=='U'||getType1(l)=='J') return -1;
    return getPart(getValxx(mem,l,l+3),19,15);
}
int getRs2(int l)
{
    if(getType1(l)=='U'||getType1(l)=='J'||getType1(l)=='I') return -1;
    return getPart(getValxx(mem,l,l+3),24,20);
}
int getDest(int l)
{
    char t1=getType1(l);
    if(t1=='S') return getRs2(l);
    else if(t1=='B') return 0;
    else return getRd(l);
}
void getAllCommands()
{
    std::ios::sync_with_stdio(false);
    int tp=0;
    char ts[20];
    while(std::cin>>ts)
    {
        if(ts[0]=='@') tp=getValx(ts,1,strlen(ts)-1);
        else
        {
            int v1=('0'<=ts[0]&&ts[0]<='9')?ts[0]-'0':ts[0]-'A'+10;
            int v2=('0'<=ts[1]&&ts[1]<='9')?ts[1]-'0':ts[1]-'A'+10;
            mem[tp++]=v1<<4|v2;
        }
    }
}

int reg[32],pc;
int main()
{
    freopen("task.data","r",stdin);
    freopen("naive.out","w",stdout);
    getAllCommands();
    while(1)
    {
        printf("%d\n",pc);
        for(int i=0;i<32;i++) printf("%d ",reg[i]);printf("\n");
        if(getValxx(mem,pc,pc+3)==0x0ff00513)
        {
            printf("%u",(unsigned int)(reg[10])&255u);
            return 0;
        }
        int typ=getType2(pc);
        switch(typ)
        {
            case LUI:reg[getRd(pc)]=getImm(pc);pc+=4;break;
            case AUIPC:reg[getRd(pc)]=pc+getImm(pc);pc+=4;break;
            case JAL:reg[getRd(pc)]=pc+4;pc+=getImm(pc);break;
            case JALR:{int tmp=pc;pc=reg[getRs1(pc)]+getImm(pc);reg[getRd(tmp)]=tmp+4;break;}
            case BEQ:if(reg[getRs1(pc)]==reg[getRs2(pc)]) pc+=getImm(pc);else pc+=4;break;
            case BNE:if(reg[getRs1(pc)]!=reg[getRs2(pc)]) pc+=getImm(pc);else pc+=4;break;
            case BLT:if(reg[getRs1(pc)]<reg[getRs2(pc)]) pc+=getImm(pc);else pc+=4;break;
            case BGE:if(reg[getRs1(pc)]>=reg[getRs2(pc)]) pc+=getImm(pc);else pc+=4;break;
            case BLTU:if((unsigned int)reg[getRs1(pc)]<(unsigned int)reg[getRs2(pc)]) pc+=getImm(pc);else pc+=4;break;
            case BGEU:if((unsigned int)reg[getRs1(pc)]>=(unsigned int)reg[getRs2(pc)]) pc+=getImm(pc);else pc+=4;break;
            case LB:
            {
                int dir=reg[getRs1(pc)]+getImm(pc);
                reg[getRd(pc)]=sext(getValxx(mem,dir,dir),8);
                pc+=4;break;
            }
            case LH:
            {
                int dir=reg[getRs1(pc)]+getImm(pc);
                reg[getRd(pc)]=sext(getValxx(mem,dir,dir+1),16);
                pc+=4;break;
            }
            case LW:
            {
                int dir=reg[getRs1(pc)]+getImm(pc);
                reg[getRd(pc)]=getValxx(mem,dir,dir+3);
                pc+=4;break;
            }
            case LBU:
            {
                int dir=reg[getRs1(pc)]+getImm(pc);
                reg[getRd(pc)]=getValxx(mem,dir,dir);
                pc+=4;break;
            }
            case LHU:
            {
                int dir=reg[getRs1(pc)]+getImm(pc);
                reg[getRd(pc)]=getValxx(mem,dir,dir+1);
                pc+=4;break;
            }
            case SB:
            {
                int dir=reg[getRs1(pc)]+getImm(pc),val=reg[getRs2(pc)]&255;
                for(int i=0;i<1;i++) mem[dir+i]=val;
                pc+=4;break;
            }
            case SH:
            {
                int dir=reg[getRs1(pc)]+getImm(pc),val=reg[getRs2(pc)]&65535;
                for(int i=0;i<2;i++) mem[dir+i]=(val>>(i*8))&255;
                pc+=4;break;
            }
            case SW:
            {
                int dir=reg[getRs1(pc)]+getImm(pc),val=reg[getRs2(pc)];
                for(int i=0;i<4;i++) mem[dir+i]=(val>>(i*8))&255;
                pc+=4;break;
            }
            case ADDI:reg[getRd(pc)]=reg[getRs1(pc)]+getImm(pc);pc+=4;break;
            case SLTI:reg[getRd(pc)]=(reg[getRs1(pc)]<getImm(pc));pc+=4;break;
            case SLTIU:reg[getRd(pc)]=((unsigned int)reg[getRs1(pc)]<(unsigned int)getImm(pc));pc+=4;break;
            case XORI:reg[getRd(pc)]=reg[getRs1(pc)]^getImm(pc);pc+=4;break;
            case ORI:reg[getRd(pc)]=reg[getRs1(pc)]|getImm(pc);pc+=4;break;
            case ANDI:reg[getRd(pc)]=reg[getRs1(pc)]&getImm(pc);pc+=4;break;
            case SLLI:reg[getRd(pc)]=reg[getRs1(pc)]<<(getImm(pc)&31);pc+=4;break;
            case SRLI:reg[getRd(pc)]=((unsigned int)reg[getRs1(pc)])>>(getImm(pc)&31);pc+=4;break;
            case SRAI:reg[getRd(pc)]=reg[getRs1(pc)]>>(getImm(pc)&31);pc+=4;break;
            case ADD:reg[getRd(pc)]=reg[getRs1(pc)]+reg[getRs2(pc)];pc+=4;break;
            case SUB:reg[getRd(pc)]=reg[getRs1(pc)]-reg[getRs2(pc)];pc+=4;break;
            case SLL:reg[getRd(pc)]=reg[getRs1(pc)]<<(reg[getRs2(pc)]&31);pc+=4;break;
            case SLT:reg[getRd(pc)]=(reg[getRs1(pc)]<reg[getRs2(pc)]);pc+=4;break;
            case SLTU:reg[getRd(pc)]=((unsigned int)reg[getRs1(pc)]<(unsigned int)reg[getRs2(pc)]);pc+=4;break;
            case XOR:reg[getRd(pc)]=reg[getRs1(pc)]^reg[getRs2(pc)];pc+=4;break;
            case SRL:reg[getRd(pc)]=((unsigned int)reg[getRs1(pc)])>>(reg[getRs2(pc)]&31);pc+=4;break;
            case SRA:reg[getRd(pc)]=reg[getRs1(pc)]>>(reg[getRs2(pc)]&31);pc+=4;break;
            case OR:reg[getRd(pc)]=reg[getRs1(pc)]|reg[getRs2(pc)];pc+=4;break;
            case AND:reg[getRd(pc)]=reg[getRs1(pc)]&reg[getRs2(pc)];pc+=4;break;
        }
        reg[0]=0;
    }    
}