/*
 * Trigger.c
 *
 * Created: 7/15/2012 4:07:00 PM
 *  Author: mike
 */
#include <stdbool.h>
#include "Globals.h"
#include "Common.h"
#include "Solenoid.h"

uint32_t trigger_activeTime = 0;
uint8_t trigger_roundsFired = 0;
bool trigger_pulled = false;
bool trigger_burstComplete = true;

void trigger_singleShot(uint32_t *millis);
void trigger_fullAuto(uint32_t *millis);
void trigger_autoResponse(uint32_t *millis);
void trigger_burst(uint32_t *millis);
void trigger_roundComplete();

void (*fireMethod)(uint32_t *millis) = &trigger_fullAuto;

bool triggerHeld() {
	return ((PINB & (1 << PINB2)) <= 0) || ((PINA & (1 << PINA6)) <= 0);
	//return (pinHasInput(TRIGGER_PIN_1) || pinHasInput(TRIGGER_PIN_2));
}

bool triggerReleased() {
	return ((PINB & (1 << PINB2)) > 0) && ((PINA & (1 << PINA6)) > 0);
	//return (!pinHasInput(TRIGGER_PIN_1) && !pinHasInput(TRIGGER_PIN_2));
}

void trigger_run(uint32_t *millis) {
	fireMethod(millis);
}

bool checkPullDebounce(uint32_t *millis) {
	return (((*millis) - trigger_activeTime) >= PULL_DEBOUNCE);
}

bool checkReleaseDebounce(uint32_t *millis) {
	return (((*millis) - trigger_activeTime) >= RELEASE_DEBOUNCE);
}

void trigger_changeMode() {
	// Stop any active firing
	trigger_burstComplete = true;
	trigger_pulled = false;
	
	// 0 - Full Auto
	// 1 - Burst
	// 2 - Auto Response
	if (FIRING_MODE == 0) {
		fireMethod = &trigger_fullAuto;
	} else if (FIRING_MODE == 1) {
		fireMethod = &trigger_burst;
	} else if (FIRING_MODE == 2) {
		fireMethod = &trigger_autoResponse;
	} else {
		fireMethod = &trigger_singleShot;
	}
}

void trigger_singleShot(uint32_t *millis) {

	// Trigger Pulled
	if (!trigger_pulled &&
		triggerHeld() &&
		checkReleaseDebounce(millis)) {
		
		trigger_pulled = true;
		trigger_activeTime = (*millis);
		solenoid_reset();
	}

	// Trigger Release
	if (trigger_pulled == true && triggerReleased() && checkPullDebounce(millis)) {

		trigger_pulled = false;
		trigger_activeTime = (*millis);
	}

	solenoid_run(millis);
}

void trigger_fullAuto(uint32_t *millis) {

	// Trigger Pulled
	if (!trigger_pulled && 
		triggerHeld() &&
		checkReleaseDebounce(millis)) {

		trigger_pulled = true;
		trigger_activeTime = (*millis);
		trigger_burstComplete = false;
		solenoid_reset();
	}

	// Trigger Held
	if (trigger_pulled == true &&
		triggerHeld() &&
		checkPullDebounce(millis) &&
		(((*millis) - trigger_activeTime) >= ROUND_DELAY)) {
	
		trigger_activeTime = (*millis);	
		solenoid_reset();
	}

	// Trigger Release
	if (trigger_pulled && triggerReleased() && checkPullDebounce(millis)) {
		trigger_pulled = false;
	}

	solenoid_run(millis);
}

void trigger_autoResponse(uint32_t *millis) {

	// Trigger Pulled
	if (!trigger_pulled && 
		triggerHeld() && 
		checkReleaseDebounce(millis)) {
			
		trigger_pulled = true;
		trigger_activeTime = (*millis);
		solenoid_reset();
		solenoid_run(millis);
	}

	// Trigger Release
	if (trigger_pulled == true && 
		triggerReleased() && 
		checkPullDebounce(millis)) {
			
		trigger_pulled = false;
		trigger_activeTime = (*millis);
		
		solenoid_reset();
	}

	solenoid_run(millis);
}

void trigger_burst(uint32_t *millis) {

	// Trigger Pulled
	if (!trigger_pulled &&
		triggerHeld() &&
		checkPullDebounce(millis)) {
		
		trigger_pulled = true;
		trigger_activeTime = (*millis);
		trigger_burstComplete = false;
		trigger_roundsFired = 0;
		solenoid_reset();
	}

	// Trigger Release
	if (trigger_pulled == true &&
		triggerReleased() &&
		checkPullDebounce(millis)) {
		
		trigger_pulled = false;
		trigger_activeTime = (*millis);
	}

	if (!trigger_burstComplete && 
		(((*millis) - trigger_activeTime) >= ROUND_DELAY)) {

		trigger_activeTime = (*millis);
		solenoid_reset();
	}

	solenoid_run_callback(millis, &trigger_roundComplete);
}

void trigger_roundComplete() {
	trigger_roundsFired++;
	if (trigger_roundsFired == BURST_SIZE) {
		trigger_burstComplete = true;
	}
}