// Math
u64 state[2];
ex void csl_math_setseed(u64 seed) {
	auto splitmix = [](u64& x) {
		u64 z = (x += 0x9e3779b97f4a7c15ULL);
		z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
		z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
		return z ^ (z >> 31);
		};

	u64 x = seed;
	state[0] = splitmix(x);
	state[1] = splitmix(x);
}
ex u64 csl_math_randomseed() {
	return __rdtsc();
}
ex i32 csl_math_random(i32 lower, i32 upper) {
	u64 s1 = state[0];
	u64 s0 = state[1];

	state[0] = s0;
	s1 ^= s1 << 23;
	s1 ^= s1 >> 17;
	s1 ^= s0;
	s1 ^= s0 >> 26;
	state[1] = s1;

	u64 v = s1 + s0;
	i32 size = upper - lower + 1;
	return (v % size) + lower;
}