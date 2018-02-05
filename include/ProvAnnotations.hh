#ifndef PROV_ANNOTATIONS_H
#define PROV_ANNOTATIONS_H

//Annotations for parameters
#define INPUT_ANNOTATION	"INPUT"
#define _IN_ __attribute__((annotate(INPUT_ANNOTATION)))

#define OUTPUT_ANNOTATION	"OUTPUT"
#define _OUT_ __attribute__((annotate(OUTPUT_ANNOTATION)))

#define MEMORY_ANNOTATION	"MEMORY"
#define _MEM_ __attribute__((annotate(MEMORY_ANNOTATION)))

#define FILE_ANNOTATION		"FILE"
#define _FILE_ __attribute__((annotate(FILE_ANNOTATION)))


//Annotations for functions

#define SOURCE_ANNO "SOURCE"
#define _SOURCE_ __attribute__((annotate(SOURCE_ANNO)))

#define SINK_ANNO	"SINK"
#define _SINK_ __attribute__((annotate(SINK_ANNO)))


#endif /* PROV_ANNOTATIONS_H */
