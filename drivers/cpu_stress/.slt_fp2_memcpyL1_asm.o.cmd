cmd_drivers/cpu_stress/slt_fp2_memcpyL1_asm.o := ./scripts/gcc-wrapper.py /home/sat/android1/aarch64-linux-android-4.9/bin/aarch64-linux-android-gcc -Wp,-MD,drivers/cpu_stress/.slt_fp2_memcpyL1_asm.o.d  -nostdinc -isystem /home/sat/android1/aarch64-linux-android-4.9/bin/../lib/gcc/aarch64-linux-android/4.9.x/include -I./arch/arm64/include -Iarch/arm64/include/generated  -Iinclude -I./arch/arm64/include/uapi -Iarch/arm64/include/generated/uapi -I./include/uapi -Iinclude/generated/uapi -include ./include/linux/kconfig.h -D__KERNEL__ -mlittle-endian  -D__ASSEMBLY__ -Wa,-gdwarf-2          -c -o drivers/cpu_stress/slt_fp2_memcpyL1_asm.o drivers/cpu_stress/slt_fp2_memcpyL1_asm.S

source_drivers/cpu_stress/slt_fp2_memcpyL1_asm.o := drivers/cpu_stress/slt_fp2_memcpyL1_asm.S

deps_drivers/cpu_stress/slt_fp2_memcpyL1_asm.o := \

drivers/cpu_stress/slt_fp2_memcpyL1_asm.o: $(deps_drivers/cpu_stress/slt_fp2_memcpyL1_asm.o)

$(deps_drivers/cpu_stress/slt_fp2_memcpyL1_asm.o):
