#include"mainwindow.h"
#include"ui_mainwindow.h"
#include"pcb.h"
#include"mem_block.h"
#include<QMessageBox>
#include<QtDebug>
#include<string>
#include<vector>
#include<algorithm>
#include<QString>
using namespace std;

QString get_mem_info(const mem_block& mb);
bool check_add_job_valid(int pid,int t,int pri,int mem);
bool cmp_mem_base(mem_block& lhs,mem_block& rhs);
void allocate_mem_block(int index,int mem);
int search_for_mem_block(int p_mem);
void add_job(int new_pid,int new_time,int new_priority,int new_memory);
bool add_job(pcb p);
bool cmp_pcb_pri(const pcb& lhs,const pcb& rhs);
bool is_insq(int pid);
void merge_mem_block(int base);
void delete_job();
void cpu_run();
void job_scheduling();
QString get_p_info(const pcb& p);
QString get_mem_info(const mem_block& mb);
const int inf = -0x3f3f3f3f;
static pcb fp(inf,inf,inf,inf,false,inf,inf,inf,inf);
vector<pcb> ready_queue;  //就绪队列
vector<pcb> standby_queue;//后备队列
vector<pcb> suspend_queue;//挂起队列
vector<mem_block> free_mem_link; //空闲内存链表
const int max_ready = 4;  //道数
const int max_mem = 100;  //内存空间100
const int osk_mem = 20;   //内核总占内存20
const int max_processor = 2; //两核

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool check_add_job_valid(int pid,int t,int pri,int mem)
{
    qDebug() << "bool check_add_job_valid(int pid,int t,int pri,int mem)";
    if(pid <= 0 || t <= 0 || pri <= 0 || mem <= 0){
        QMessageBox::critical(NULL,"Reject","Args should be greater than zero!",QMessageBox::Yes,QMessageBox::Yes);
        return false;
    }else if(mem > (max_mem - osk_mem)){
        QMessageBox::critical(NULL,"Reject","Job is too large!", QMessageBox::Yes,QMessageBox::Yes);
        return false;
    }else if(pri < t){
        QMessageBox::critical(NULL,"Reject","Priority should be greater than time!", QMessageBox::Yes, QMessageBox::Yes);
        return false;
    }
    for(int i = 0;i < ready_queue.size(); ++i){
        if(ready_queue[i].pid == pid){
            QMessageBox::critical(NULL,"Reject","Pid has already existed!", QMessageBox::Yes,QMessageBox::Yes);
            return false;
        }
    }
    for(int i = 0;i < standby_queue.size(); ++i){
        if(standby_queue[i].pid == pid){
            QMessageBox::critical(NULL,"Reject","Pid has already existed!", QMessageBox::Yes,QMessageBox::Yes);
            return false;
        }
    }
    for(int i = 0;i < suspend_queue.size(); ++i){
        if(suspend_queue[i].pid == pid){
            QMessageBox::critical(NULL,"Reject","Pid has already existed!", QMessageBox::Yes,QMessageBox::Yes);
            return false;
        }
    }
    QMessageBox::about(NULL,"Accept","Add job successfully!");
    return true;
}

bool cmp_mem_base(mem_block& lhs,mem_block& rhs)
{
      return lhs.base < rhs.base;
}

//分配并切割内存块
void allocate_mem_block(int index,int mem)
{
    qDebug() << "void allocate_mem_block(int index,int mem)";
    free_mem_link[index].base += mem;
    free_mem_link[index].len -= mem;
    if(free_mem_link[index].len == 0){
        auto it = free_mem_link.begin() + index;
        free_mem_link.erase(it);
    }
    return ;
}

int search_for_mem_block(int p_mem)
{
    qDebug() << "int search_for_mem_block(int p_mem)";
    sort(free_mem_link.begin(),free_mem_link.end(),cmp_mem_base);
    //道满了
    if(ready_queue.size() >= max_ready){
        return -1;
    }
    //首次适配算法
    int len = free_mem_link.size();
    for(int i = 0; i < len; ++i){
        if(free_mem_link[i].len >= p_mem){
            return i;
        }
    }
    return -1;
}

void add_job(int new_pid,int new_time,int new_priority,int new_memory)
{
    qDebug() << "void add_job(int new_pid,int new_time,int new_priority,int new_memory)";
    pcb new_job_pcb(new_pid,new_time,new_priority,new_memory);
    if(check_add_job_valid(new_pid,new_time,new_priority,new_memory)){
        int mb_index;
        //尝试加入就绪队列
        if((mb_index = search_for_mem_block(new_memory)) != -1 && ready_queue.size() < max_ready){
            new_job_pcb.base = free_mem_link[mb_index].base;
            new_job_pcb.state = p_state::p_ready;
            allocate_mem_block(mb_index,new_memory);
            ready_queue.push_back(new_job_pcb);
        }else{
            //道满或内存不够，加入后备队列
            new_job_pcb.state = p_state::p_standby;
            standby_queue.push_back(new_job_pcb);
        }
    }
    return ;
}

//同步进程
void add_job(int new_pid,int new_time,int new_priority,int new_memory,int prev,int succ,int bufempty,int buffull)
{
    qDebug() << "void add_job(int new_pid,int new_time,int new_priority,int new_memory,int prev,int succ,int bufempty,int buffull)";
    pcb new_job_pcb(new_pid,new_time,new_priority,new_memory,false,prev,succ,bufempty,buffull);
    if(check_add_job_valid(new_pid,new_time,new_priority,new_memory)){
        int mb_index;
        //尝试加入就绪队列
        if((mb_index = search_for_mem_block(new_memory)) != -1 && ready_queue.size() < max_ready){
            new_job_pcb.base = free_mem_link[mb_index].base;
            new_job_pcb.state = p_state::p_ready;
            allocate_mem_block(mb_index,new_memory);
            ready_queue.push_back(new_job_pcb);
        }else{
            //道满或内存不够，加入后备队列
            new_job_pcb.state = p_state::p_standby;
            standby_queue.push_back(new_job_pcb);
        }
    }
    return ;
}

bool add_job(pcb p)
{
    qDebug() << "void add_job(pcb p)";
    int mb_index;
    if(p.is_alone == false && p.bufempty - 1 < 0){
        if(is_insq(p.pid)){
            return false;
        }
        p.state = p_state::p_standby;
        standby_queue.push_back(p);
        return false;
    }
    //尝试加入就绪队列
    if((mb_index = search_for_mem_block(p.memory)) != -1){
        p.base = free_mem_link[mb_index].base;
        p.state = p_state::p_ready;
        allocate_mem_block(mb_index,p.memory);
        ready_queue.push_back(p);
        return true;
    }else{
        //道满或内存不够，加入后备队列
        if(is_insq(p.pid)){
            return false;
        }
        p.state = p_state::p_standby;
        standby_queue.push_back(p);
    }
    return false;
}

void MainWindow::on_add_job_button_clicked()
{
    qDebug() << "void MainWindow::on_add_job_button_clicked()";
    int new_pid = ui->pid_edit->text().toInt();
    int new_time = ui->time_edit->text().toInt();
    int new_priority = ui->priority_edit->text().toInt();
    int new_memory = ui->memory_edit->text().toInt();
    if(ui->syn_edit->text().toCaseFolded() == 'n'){
        add_job(new_pid,new_time,new_priority,new_memory);
    }else{
        int new_prev = ui->prev_edit->text().toInt();
        int new_succ = ui->succ_edit->text().toInt();
        int new_bufempty = ui->bufempty_edit->text().toInt();
        int new_buffull = ui->bufful_edit->text().toInt();
        add_job(new_pid,new_time,new_priority,new_memory,new_prev,new_succ,new_bufempty,new_buffull);
    }
    return ;
}

bool cmp_pcb_pri(const pcb& lhs,const pcb& rhs)
{
    return lhs.priority > rhs.priority;
}

bool is_insq(int pid)
{
    for(int i = 0; i < standby_queue.size(); ++i){
        if(standby_queue[i].pid == pid){
            return true;
        }
    }
    return false;
}

void merge_mem_block(int base)
{
    qDebug() << "void merge_mem_block(int base)";
    int i;
    for(i = 0; i < free_mem_link.size(); ++i){
        if(free_mem_link[i].base == base){
            break;
        }
    }
    if(i-1>=0 && free_mem_link[i-1].base+free_mem_link[i-1].len == base){
        //前内存块要合并
        base = free_mem_link[i].base = free_mem_link[i-1].base;
        free_mem_link[i].len += free_mem_link[i-1].len;
        free_mem_link[i-1].valid = false;
        auto it = free_mem_link.begin() + i-1;
        free_mem_link.erase(it);
    }
    for(i = 0; i < free_mem_link.size(); ++i){
        if(free_mem_link[i].base == base){
            break;
        }
    }
    if(i+1<free_mem_link.size() && free_mem_link[i].base+free_mem_link[i].len == free_mem_link[i+1].base){
        //后内存块要合并
        free_mem_link[i].len += free_mem_link[i+1].len;
        free_mem_link[i+1].valid = false;
        auto it = free_mem_link.begin() + i+1;
        free_mem_link.erase(it);
    }
    return ;
}

void delete_job()
{
    qDebug() << "void delete_job()";
    int n = max_processor;
    while(n--){
        for(auto it = ready_queue.begin();it != ready_queue.end(); ++it){
            if(it->time == 0){
                int base = it->base;
                int mem = it->memory;
                free_mem_link.push_back(mem_block(base,mem));
                sort(free_mem_link.begin(),free_mem_link.end(),cmp_mem_base);
                merge_mem_block(base);
                ready_queue.erase(it);
                break;
            }
        }
    }
    return ;
}

pcb& search_pcb(int pid)
{
    for(int i = 0; i < ready_queue.size(); ++i){
        if(ready_queue[i].pid == pid){
            return ready_queue[i];
        }
    }
    for(int i = 0; i < standby_queue.size(); ++i){
        if(standby_queue[i].pid == pid){
            return standby_queue[i];
        }
    }
    for(int i = 0; i < suspend_queue.size(); ++i){
        if(suspend_queue[i].pid == pid){
            return suspend_queue[i];
        }
    }
    return fp;
}

//succ根据prev更新pv
void update_pv(pcb& p)
{
    if(p.prev != 0){
        search_pcb(p.prev).bufempty = p.buffull;
        search_pcb(p.prev).buffull = p.bufempty;
        return ;
    }else if(p.succ != 0){
        search_pcb(p.succ).bufempty = p.buffull;
        search_pcb(p.succ).buffull = p.bufempty;
        return ;
    }
}

void cpu_run()
{
    qDebug() << "void cpu_run()";
    sort(ready_queue.begin(),ready_queue.end(),cmp_pcb_pri);
    int j = 0;
    for(int i = 0; i < ready_queue.size(); ++i){
        if(j < max_processor){
            if(ready_queue[i].is_alone){
                ready_queue[i].state = p_state::p_running;
                --ready_queue[i].priority;
                --ready_queue[i].time;
                ++j;
            }else{
                if(ready_queue[i].bufempty - 1 >= 0){
                    ready_queue[i].state = p_state::p_running;
                    --ready_queue[i].priority;
                    --ready_queue[i].time;
                    --ready_queue[i].bufempty;
                    ++ready_queue[i].buffull;
                    update_pv(ready_queue[i]);
                    ++j;
                }
            }
        }else{
            ready_queue[i].state = p_state::p_ready;
        }
    }
    delete_job();
    return ;
}

QString get_p_info(const pcb& p)
{
    QString s = "pid: ";
    s += QString::number(p.pid,10);
    s += " priority: ";
    s += QString::number(p.priority,10);
    s += " time: ";
    s += QString::number(p.time,10);
    s += " base: ";
    s += QString::number(p.base,10);
    s += " mem: ";
    s += QString::number(p.memory,10);
    if(p.is_alone == false){
        s += " P: ";
        s += QString::number(p.bufempty,10);
        s += " V: ";
        s += QString::number(p.buffull,10);
    }
    return s;
}

QString get_mem_info(const mem_block& mb)
{
    QString s = "base: ";
    s += QString::number(mb.base,10);
    s += " len: ";
    s += QString::number(mb.len,10);
    return s;
}

void job_scheduling()
{
    qDebug() << "void job_scheduling()";
    int n = max_ready - ready_queue.size();
    while(n--){
        for(int i = 0;i < standby_queue.size(); ++i){
            if(add_job(standby_queue[i])){
                auto it = standby_queue.begin() + i;
                standby_queue.erase(it);
                break;
            }
        }
    }
    while(true){
        int i = 0;
        for(i = 0;i < ready_queue.size(); ++i){
            if(ready_queue[i].is_alone == false && ready_queue[i].bufempty - 1 < 0){
                auto it = ready_queue.begin() + i;
                free_mem_link.push_back(mem_block(ready_queue[i].base,ready_queue[i].memory));
                sort(free_mem_link.begin(),free_mem_link.end(),cmp_mem_base);
                merge_mem_block(ready_queue[i].base);
                standby_queue.push_back(ready_queue[i]);
                ready_queue.erase(it);
                break;
            }
        }
        if(i == ready_queue.size()){
            return ;
        }
    }
}

void MainWindow::on_run_button_clicked()
{
    qDebug() << "void MainWindow::on_run_button_clicked()";
    QString tips_cpu_info = "CPU Information: ";
    QString cut_off_rule = "------------------------------------------------------------------------------------------------";
    QString tips_ready_queue = "Ready Queue Information: ";
    QString tips_standby_queue = "Standby Queue Information: ";
    QString tips_suspend_queue = "Suspend Queue Information: ";
    QString tips_free_memory_info = "Free Memory Link Information: ";
    QString end_for_on_step = "---------------------------------------------End------------------------------------------------\n\n";
    QString begin_for_on_step = "---------------------------------------------Begin----------------------------------------------";
    ui->result_browser->setText(begin_for_on_step);
    ui->result_browser->append(tips_cpu_info);
    cpu_run();
    job_scheduling();
    sort(free_mem_link.begin(),free_mem_link.end(),cmp_mem_base);
    for(int i = 0;i < ready_queue.size(); ++i){
        if(i < max_processor){
            if(ready_queue[i].state == p_state::p_running){
                QString q = get_p_info(ready_queue[i]);
                ui->result_browser->append(q);
            }
        }
    }
    ui->result_browser->append(cut_off_rule);
    ui->result_browser->append(tips_ready_queue);
    for(int i = 0; i < ready_queue.size(); ++i){
        if(ready_queue[i].state == p_state::p_ready){
            QString q = get_p_info(ready_queue[i]);
            ui->result_browser->append(q);
        }
    }
    ui->result_browser->append(cut_off_rule);
    ui->result_browser->append(tips_standby_queue);
    for(int i = 0; i < standby_queue.size(); ++i){
        QString q = get_p_info(standby_queue[i]);
        ui->result_browser->append(q);
    }
    ui->result_browser->append(cut_off_rule);
    ui->result_browser->append(tips_suspend_queue);
    for(int i = 0;i < suspend_queue.size(); ++i){
        QString q = get_p_info(suspend_queue[i]);
        ui->result_browser->append(q);
    }
    ui->result_browser->append(cut_off_rule);
    ui->result_browser->append(tips_free_memory_info);
    for(int i = 0; i < free_mem_link.size(); ++i){
        QString info = get_mem_info(free_mem_link[i]);
        ui->result_browser->append(info);
    }
    ui->result_browser->append(end_for_on_step);
}

void MainWindow::on_suspend_button_clicked()
{
    qDebug() << "void MainWindow::on_suspend_button_clicked()";
    int sus_pid = ui->suspend_edit->text().toInt();
    for(int i = 0;i < ready_queue.size(); ++i){
        if(sus_pid == ready_queue[i].pid){
            int base = ready_queue[i].base;
            int mem = ready_queue[i].memory;
            free_mem_link.push_back(mem_block(base,mem));
            sort(free_mem_link.begin(),free_mem_link.end(),cmp_mem_base);
            merge_mem_block(base);
            auto it = ready_queue.begin() + i;
            ready_queue[i].state = p_state::p_suspend;
            suspend_queue.push_back(ready_queue[i]);
            ready_queue.erase(it);
            QMessageBox::about(NULL,"Accept","Suspend job successfully!");
            job_scheduling();
            return ;
        }
    }
    QMessageBox::critical(NULL,"Reject","No matched job!", QMessageBox::Yes, QMessageBox::Yes);
    return ;
}

void MainWindow::on_unsuspend_button_clicked()
{
    qDebug() << "void MainWindow::on_unsuspend_button_clicked()";
    int unsus_pid = ui->unsuspend_edit->text().toInt();
    for(int i = 0; i < suspend_queue.size(); ++i){
        if(suspend_queue[i].pid == unsus_pid){
            add_job(suspend_queue[i]);
            auto it = suspend_queue.begin() + i;
            suspend_queue.erase(it);
            QMessageBox::about(NULL,"Accept","Unsuspend job successfully!");
            return ;
        }
    }
    QMessageBox::critical(NULL,"Reject","Can't find this job!", QMessageBox::Yes, QMessageBox::Yes);
}

void MainWindow::on_start_button_clicked()
{
    qDebug() << "void MainWindow::on_start_button_clicked()";
    ui->add_job_button->setEnabled(true);
    ui->run_button->setEnabled(true);
    ui->suspend_button->setEnabled(true);
    ui->unsuspend_button->setEnabled(true);
    ui->start_button->setEnabled(false);
    ui->terminate_button->setEnabled(true);
    free_mem_link.push_back(mem_block(osk_mem,max_mem-osk_mem));
    return ;
}

void MainWindow::on_terminate_button_clicked()
{
    qDebug() << "void MainWindow::on_terminate_button_clicked()";
    ui->start_button->setEnabled(true);
    ui->add_job_button->setEnabled(false);
    ui->run_button->setEnabled(false);
    ui->suspend_button->setEnabled(false);
    ui->unsuspend_button->setEnabled(false);
    ui->terminate_button->setEnabled(false);
    ready_queue.clear();
    standby_queue.clear();
    suspend_queue.clear();
    free_mem_link.clear();
    return ;
}
