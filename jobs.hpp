// Jobs
ex csl_jobs csl_jobs_create() {
	csl_allocation al = csl_mem_alloc(sizeof(csl_queue) * 2);
	u32 sz = sizeof(ptr) + sizeof(csl_allocation);
	*(csl_vaddr*)((char*)al) = csl_vaddr_store(csl_queue_create(sz));
	*(csl_vaddr*)((char*)al + sizeof(csl_vaddr)) = csl_vaddr_store(csl_queue_create(sz));
	return al;
}
csl_allocation csl_jobs_build(ptr f, csl_allocation data) {
	csl_allocation al = csl_mem_alloc(sizeof(ptr) + sizeof(csl_allocation));
	*(ptr*)al = f;
	*(csl_allocation*)((char*)al + sizeof(ptr)) = data;
	return al;
}
ex void csl_jobs_queuepriority(csl_jobs j, ptr f, csl_allocation data) {
	csl_allocation job = csl_jobs_build(f, data);
	csl_queue_enqueue(csl_vaddr_get(*(csl_vaddr*)((char*)j)), job);
	csl_mem_free(job);
}
ex void csl_jobs_queue(csl_jobs j, ptr f, csl_allocation data) {
	csl_allocation job = csl_jobs_build(f, data);
	csl_queue_enqueue(csl_vaddr_get(*(csl_vaddr*)((char*)j + sizeof(csl_vaddr))), job);
	csl_mem_free(job);
}
void csl_job_runbuild(csl_allocation b) {
	ptr fp = *(ptr*)b;
	void(*f)(csl_allocation) = (void(*)(csl_allocation))fp;
	csl_allocation data = *(csl_allocation*)((char*)b + sizeof(ptr));
	csl_mem_free(b);
	f(data);
}
ex bool csl_jobs_run(csl_jobs j) {
	void* job = csl_queue_get(csl_vaddr_get(*(csl_vaddr*)((char*)j)));
	if (job != 0) {
		csl_job_runbuild(job);
		return true;
	}
	job = csl_queue_get(csl_vaddr_get(*(csl_vaddr*)((char*)j + sizeof(csl_vaddr))));
	if (job != 0) {
		csl_job_runbuild(job);
		return true;
	}
	return false;
}
ex void csl_jobs_destroy(csl_jobs j) {
	csl_queue_destroy(csl_vaddr_get(*(csl_vaddr*)((char*)j)));
	csl_queue_destroy(csl_vaddr_get(*(csl_vaddr*)((char*)j + sizeof(csl_vaddr))));
	csl_mem_free(j);
}