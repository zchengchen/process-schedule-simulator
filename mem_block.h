#ifndef MEM_BLOCK_H
#define MEM_BLOCK_H

class mem_block{
public:
    int base;//内存地址起始
    int len;//长度
    bool valid;//是否被分配（其实没有用）
    mem_block(int b,int l,bool v = true):base(b),len(l),valid(v){ };
};

#endif // MEM_BLOCK_H
