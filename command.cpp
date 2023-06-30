#include <iostream>
#include <cstring>
#include "parser.cpp"
enum counter{stronglyNotTaken,weaklyNotTaken,weaklyTaken,stronglyTaken};
class BranchPredictor
{
    const static int N=5000,MOD=307;
    int freq[N];
    int getHash(int pc){return 1ll*pc*MOD%N;}
public:
    BranchPredictor()
    {
        for(int i=0;i<N;i++) freq[i]=weaklyNotTaken;
    }
    bool getPredict(int pc)
    {
        int id=getHash(pc);
        if(freq[id]==weaklyTaken||freq[id]==stronglyTaken) return true;
        else return false;
    }
    void changeFreq(bool is,int pc)
    {
        int id=getHash(pc);
        if(is&&freq[id]!=stronglyTaken) freq[id]++;
        if(!is&&freq[id]!=stronglyNotTaken) freq[id]--;
    }
} bP;
class CommandQueue
{
    bool used;
    int pc,pc2;
public:
    CommandQueue(){pc=pc2=0;}
    void nextClock(){pc=pc2;used=false;}
    void modify(int v){pc2=v;used=true;}
    int getpc(){return pc;}
    int getCommand()
    {
        int ret=pc;
        if(used||getType1(pc)=='?') return -1;
        if(getType1(pc)=='B')
        {
            if(bP.getPredict(pc)) pc2+=getImm(pc);
            else pc2+=4;
        }
        else pc2+=4;
        return ret;
    }
} cQ;