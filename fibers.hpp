// Fibers

csl_allocation csl_fiber_routine(csl_jobs j) {
	while (csl_atomic_load(&csl_globals->fibers)) {
		if (!csl_jobs_run(j)) {
			csl_time_pause();
		}
	}
	return 0;
}
void csl_fiber_shutdown() {
	csl_atomic_store(&csl_globals->fibers, 0);
	for (u32 i = 0; i < csl_globals->threads; i++) {
		csl_thread th = *(csl_thread*)((char*)csl_globals->fiberslist + (sizeof(csl_thread) * i));
		csl_thread_merge(th);
	}
	csl_mem_free(csl_globals->fiberslist);
	csl_jobs_destroy(csl_vaddr_get(csl_globals->jobs));
}
void csl_fiber_init() {
	csl_globals->jobs = csl_vaddr_store(csl_jobs_create());
	csl_globals->threads = csl_thread_count() - 1;
	csl_globals->fiberslist = csl_mem_alloc(sizeof(csl_thread) * csl_globals->threads);
	for (u32 i = 0; i < csl_globals->threads; i++) {
		*(csl_thread*)((char*)csl_globals->fiberslist + (sizeof(csl_thread) * i)) = csl_thread_branch(csl_fiber_routine, csl_vaddr_get(csl_globals->jobs));
	}
	csl_atomic_store(&csl_globals->fibers, 1);
}
void csl_fiber_queue(ptr f, csl_allocation data) {
	csl_jobs_queue(csl_vaddr_get(csl_globals->jobs), f, data);
}
void csl_fiber_queuepriority(ptr f, csl_allocation data) {
	csl_jobs_queuepriority(csl_vaddr_get(csl_globals->jobs), f, data);
}