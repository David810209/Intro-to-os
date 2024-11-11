rm -rf multilevelBF.so free_list_status.txt memory_layout.txt
gcc -shared -fPIC multilevelBF.c -o multilevelBF.so
#  gcc -shared -fPIC t.c -o t.so
LD_PRELOAD=./multilevelBF.so ./main
# LD_PRELOAD=./ray.so ./main
