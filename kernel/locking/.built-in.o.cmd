cmd_kernel/locking/built-in.o :=  /home/sat/android1/aarch64-linux-android-4.9/bin/aarch64-linux-android-ld -EL    -r -o kernel/locking/built-in.o kernel/locking/mutex.o kernel/locking/semaphore.o kernel/locking/rwsem.o kernel/locking/mcs_spinlock.o kernel/locking/mutex-debug.o kernel/locking/spinlock.o kernel/locking/lglock.o kernel/locking/rtmutex.o kernel/locking/spinlock_debug.o kernel/locking/rwsem-xadd.o 