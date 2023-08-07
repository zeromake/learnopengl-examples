#define sg_alloc_image_smp(bindings, image_index, smp_index) bindings.images[image_index] = sg_alloc_image();\
bindings.samplers[smp_index] = sg_alloc_sampler()
