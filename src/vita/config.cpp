#include "config.h"

TUserConfiguration UserConfiguration;

TUserConfiguration::TUserConfiguration() {
	soundEnabled		= true;
	jitEnabled			= true;
	fpsCounterEnabled	= false;
	frameSkip			= 1;
	threadedRendering	= true;
}