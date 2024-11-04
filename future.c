#include <assert.h>
#include "plug_internal.h"

// -------------
// Futures 
// -------------
void CreateFuture(State* state, float delay, void** reference, void* future_value, void(*callable)(void)) {
	RenderInfo* render_info = &state->render_info;

	Future* future = (Future*)malloc(sizeof(Future));
	future->delay = delay;
	future->reference = reference;
	future->future_value = future_value;
	future->callable = callable;
	for (size_t i = 0; i < MAX_FUTURES; i++) {
		if (render_info->futures[i] == NULL) {
			render_info->futures[i] = future;
			return;
		}		
	}
	assert(false && "Future queue is full");
}

static void FutureStep(Future* future, float dt) {
	if (future->delay > 0) {
		future->delay = MaxFloat(0, future->delay - dt);
	}
	if (future->delay > 0) {
		return;
	}
	if (future->callable != NULL) {
		future->callable();
	}
	if (future->reference != NULL) {
		*future->reference = future->future_value;
	}
}

bool StepFutures(State* state) {
	Future** futures = state->render_info.futures; 
	bool has_events = false;
	float dt = GetFrameTime();
	for (int i = 0; i < MAX_FUTURES; i++) {
		Future* future = futures[i];
		if (future == NULL) {
			continue;
		} 
		has_events = true;
		FutureStep(future, dt);

		if (future->delay == 0) {
			futures[i] = NULL;
			FutureFree(future);
		}
	}
	return has_events;
}

void FutureFree(Future* future) {
	free(future);
}
