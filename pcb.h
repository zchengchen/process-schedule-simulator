#ifndef PCB_H
#define PCB_H
#include<string>
using namespace std;

typedef enum {p_ready,p_standby,p_running,p_suspend} p_state;
class pcb {
public:
    int pid; //进程id
    int time;//运行时间
    int priority;//优先级
    p_state state;//状态
    int base;//基址
    int memory;//内存大小
    bool is_alone;//是否为独立进程
    int bufempty;//p
    int buffull;//v
    int prev;//前驱
    int succ;//后继
    pcb(int p=0,int t=0,int pri=0,int m=0,bool is_a = true,int pr = 0,int s = 0,int be = 0,int bf = 0):
        pid(p),time(t),priority(pri),state(p_state::p_standby),base(-1),memory(m), is_alone(is_a),prev(pr),succ(s),bufempty(be),buffull(bf){}
};

#endif // PCB_H
