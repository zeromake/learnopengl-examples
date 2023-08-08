static const sg_sampler_desc global_sampler_desc = {
    .wrap_u = SG_WRAP_REPEAT,
    .wrap_v = SG_WRAP_REPEAT,
    .min_filter = SG_FILTER_LINEAR,
    .mag_filter = SG_FILTER_LINEAR,
    .compare = SG_COMPAREFUNC_NEVER,
};

#define sg_alloc_image_smp(bindings, image_index, smp_index) bindings.images[image_index] = sg_alloc_image();\
bindings.samplers[smp_index] = sg_alloc_sampler();\
sg_init_sampler(bindings.samplers[smp_index], &global_sampler_desc);

