#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "ui_internal.h"


void CreateAnimation(State* state, double end, double* value, EasingFunction easing, float duration, float delay, StyleOverride* style_override) {
	RenderInfo* render_info = &state->render_info;
	Animation* animation = (Animation*)malloc(sizeof(Animation));
	animation->easing = easing;
	animation->percent = 0;
	animation->duration = duration; 
	animation->delay = delay;
	animation->start_val = *value;
	animation->end_val = end;
	animation->value = value;
	animation->style_override = style_override;
	for (int i = 0; i < MAX_ANIMATIONS; i++) {
		if (render_info->animations[i] == NULL) {
			render_info->animations[i] = animation;
			return;
		}
	}
	assert(false && "Animation queue is full");
}

static void StepAnimation(Animation* animation, float dt) {
	if (animation->delay > 0) {
		float remaining_time = animation->delay - dt;
		animation->delay = MaxFloat(0, remaining_time);
		dt += remaining_time;
	}
	if (animation->delay > 0) {
		return;
	}
	if (animation->duration == 0) {
		animation->percent = 1.0F;
		*animation->value = animation->end_val;
		return;
	}
	animation->percent = MinDouble(dt / animation->duration + animation->percent, 1.0F);
	float percent = animation->percent;
	switch (animation->easing) {
		case LINEAR:
			break;
		case IN_N_OUT:
			percent = ParametricBlend(percent);
			break;
		case IN_N_BACK:
			percent = SinInAndBack(percent);
			break;
		default:
			fprintf(stderr, "Unexpected easing function %d\n", animation->easing);
	}
	float val = animation->start_val + (animation->end_val - animation->start_val) * percent;
	*animation->value = val;
}

void AnimationFree(State* state, Animation* animation) {
	if (animation->style_override != NULL) {
		StyleOverrideDestroy(state, animation->style_override);
	}
	free(animation);
}

bool StepAnimations(State* state) {
	// @return bool: True if there are animations left
	RenderInfo* render_info = &state->render_info;
	Animation** animations = render_info->animations; 
	bool has_events = false;
	float dt = GetFrameTime();
	for (int i = 0; i < MAX_ANIMATIONS; i++) {
		Animation* animation = animations[i];
		if (animation == NULL) {
			continue;
		} 
		has_events = true;
		if (animation->percent == 1) {
			animations[i] = NULL;
			AnimationFree(state, animation);
		} else {
			StepAnimation(animation, dt);
		}

	}
	return has_events;
}

