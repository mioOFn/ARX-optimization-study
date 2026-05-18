# ARX密码性能优化研究

## 项目目录结构
ARX-optimization-study/    
├── README.md                         # 项目说明   
│
├── bench_arx_microarch/                             # 微基准实验   
│   ├── bench_arx_microarch.c                          
│   └── analyze_arx_microarch.py                      # 数据分析  
│   
├── Blake_Reference_Implementation/                # Blake以及其变体Blake_sip   
│   ├── blake_ref.h                                
│   ├── blake_ref.c                               # 原Blake参考实现   
│   ├── bench_blake512_hash_sipg.c                # Blake_sip哈希函数测试   
│   ├── bench_blake512_hash.c                     # Blake512哈希函数测试  
│   ├── bench_blake512_compress64.c               # Blake512压缩函数测试     
│   └── bench_blake512_compress64_sipg.c          # Blake_sip压缩函数测试      
│    
├── Skein_Reference_Implementation/                       # Skein及其变体   
│   ├── skein_port.h                    
│   ├── skein_debug.h                    
│   ├── skein_debug.c                      
│   ├── skein_block.c                    
│   ├── skein.h                   
│   ├── skein.c    
│   ├── SHA3api_ref.h    
│   ├── SHA3api_ref.c      
│   ├── brg_types.h      
│   ├── brg_endian.h                          # 以上为Skein参考实现   
│   ├── bench_skein512_process_block.c        # Skein512压缩函数测试  
│   ├── bench_skein512_hash.c                 # Skein512哈希函数测试  
│   ├── bench_skein256_hash.c     
│   ├── ballet_ubi128_256.h                   #Ballet-UBI实现（Ballet嵌入UBI框架）  
│   ├── ballet_ubi128_256.c     
│   └── bench_ballet_ubi128_256_hash.c        #Ballet-UBI性能测试    
                    
