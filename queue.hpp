// Queue
ex csl_queue csl_queue_create(u32 isz) {
	csl_list l = csl_list_create(isz);
	csl_allocation al = csl_mem_alloc(sizeof(csl_mutex) + sizeof(csl_vaddr) + sizeof(csl_atomic) + sizeof(csl_atomic));
	*(csl_mutex*)al = 0;
	*(csl_vaddr*)((char*)al + sizeof(csl_mutex)) = csl_vaddr_store(l);
	*(csl_atomic*)((char*)al + sizeof(csl_mutex) + sizeof(csl_vaddr)) = true;
	*(csl_atomic*)((char*)al + sizeof(csl_mutex) + sizeof(csl_vaddr) + sizeof(csl_atomic)) = 0;
	return al;
}
void csl_queue__destroy(csl_queue q) {
	csl_list_destroy(csl_vaddr_get(*(csl_vaddr*)((char*)q + sizeof(csl_mutex))));
	csl_mem_free(q);
}
ex void csl_queue_enqueue(csl_queue q, void* obj) {
	csl_atomic* users = (csl_atomic*)((char*)q + sizeof(csl_mutex) + sizeof(csl_vaddr) + sizeof(csl_atomic));
	csl_atomic_inc(users);
	csl_atomic* alive = (csl_atomic*)((char*)q + sizeof(csl_mutex) + sizeof(csl_vaddr));
	if (!csl_atomic_load(alive)) {
		csl_atomic_dec(users);
		return;
	}
	csl_mutex_acquire((csl_mutex*)q);
	csl_list l = csl_vaddr_get(*(csl_vaddr*)((char*)q + sizeof(csl_mutex)));
	csl_list_append(l, obj);
	csl_mutex_release((csl_mutex*)q);
	csl_atomic_dec(users);
}
ex void* csl_queue_get(csl_queue q) {
	csl_atomic* users = (csl_atomic*)((char*)q + sizeof(csl_mutex) + sizeof(csl_vaddr) + sizeof(csl_atomic));
	csl_atomic_inc(users);
	csl_atomic* alive = (csl_atomic*)((char*)q + sizeof(csl_mutex) + sizeof(csl_vaddr));
	if (!csl_atomic_load(alive)) {
		csl_atomic_dec(users);
		return 0;
	}
	csl_mutex_acquire((csl_mutex*)q);
	csl_list l = csl_vaddr_get(*(csl_vaddr*)((char*)q + sizeof(csl_mutex)));
	csl_allocation al = 0;
	if (csl_list_size(l) > 0) {
		void* obj = csl_list_get(l, 0);
		u32 osz = csl_list_objsize(l);
		al = csl_mem_alloc(osz);
		csl_mem_cpy(al, obj, osz);
		csl_list_erase(l, 0);
	}
	csl_mutex_release((csl_mutex*)q);
	csl_atomic_dec(users);
	return al;
}
ex void csl_queue_destroy(csl_queue q) {
	csl_atomic* alive = (csl_atomic*)((char*)q + sizeof(csl_mutex) + sizeof(csl_vaddr));
	csl_atomic_store(alive, false);
	csl_atomic* users = (csl_atomic*)((char*)q + sizeof(csl_mutex) + sizeof(csl_vaddr) + sizeof(csl_atomic));
	while (csl_atomic_load(users) != 0) {
		csl_time_pause();
	}
	csl_queue__destroy(q);
}