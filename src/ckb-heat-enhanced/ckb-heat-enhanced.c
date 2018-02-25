/*
Copyright (c) 2017 Nate Simon (github.com: @ion201)
Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to the following
conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#define ENABLE_DEBUG_PRINT 0

#include <stdio.h>
#include <limits.h>
#include <stdbool.h>
#include <unistd.h>
#include "../ckb-anim.h"

#define CKB_HEAT_ENHANCED_RELEASE_VERSION 1.0

// Setup default parameters
#define PARAM_FADE_TIME_STRING  "fadetime"
#define PARAM_FADE_TIME_DEFAULT 500
#define PARAM_FADE_TIME_MIN     0
#define PARAM_FADE_TIME_MAX     9999999

#define PARAM_MAX_FADE_TIME_STRING  "maxfadetime"
#define PARAM_MAX_FADE_TIME_DEFAULT 30
#define PARAM_MAX_FADE_TIME_MIN     0
#define PARAM_MAX_FADE_TIME_MAX     99999

#define PARAM_PRESSES_TO_MAX_INTENSITY_STRING   "pressestomaxintensity"
#define PARAM_PRESSES_TO_MAX_INTENSITY_DEFAULT  5
#define PARAM_PRESSES_TO_MAX_INTENSITY_MIN      1
#define PARAM_PRESSES_TO_MAX_INTENSITY_MAX      999

#define PARAM_FADE_COLOR_STRING     "fadecolor"
#define PARAM_FADE_COLOR_DEFAULT    "ffff0000"

#define SEC_TO_MS(s) (s*1000.f)
#define MS_TO_SEC(s) (s/1000.f)

typedef struct
{
    int xPos;
    int yPos;
    bool isPressed;
    long fadeTime;
} KEY_STATE_INFO_T;

KEY_STATE_INFO_T * gKeyStateInfo = NULL;

ckb_gradient gParamFadeGradient = {0};
long gParamFadeTime = PARAM_FADE_TIME_DEFAULT;
long gParamMaxFadeTime = PARAM_MAX_FADE_TIME_DEFAULT;
double gParamPressesToMaxIntensity = PARAM_PRESSES_TO_MAX_INTENSITY_DEFAULT;

#define MIN(a, b) ((a<b) ? a : b)
#define MAX(a, b) ((a>b) ? a : b)
#define CLAMP(val, min, max) ((val > max) ? max : (val < min) ? min : val)

#define STRINGIFY(s) #s
#define TO_STRING(s) STRINGIFY(s)

#if ENABLE_DEBUG_PRINT
#define DEBUG_FILE "/tmp/ckb-heat-enhanced-debug.log"
#define debugPrint(fmt, ...) do{ \
                                FILE *__f = fopen(DEBUG_FILE, "a"); \
                                fprintf(__f, fmt "\n", ##__VA_ARGS__); \
                                fclose(__f); \
                            } while(0);

#else
#define debugPrint(fmt, ...)

#endif  // ENABLE_DEBUG_PRINT

void ckb_info()
{
    CKB_NAME("Enhanced Heat Map");
    CKB_VERSION(TO_STRING(CKB_HEAT_ENHANCED_RELEASE_VERSION));
    CKB_COPYRIGHT("2017", "ion201");
    CKB_LICENSE("MIT");
    CKB_GUID("4545e97a-0ec5-474e-a0f1-e808a3555fa2");
    CKB_DESCRIPTION("TODO: Update this description");

    CKB_PARAM_AGRADIENT(PARAM_FADE_COLOR_STRING, "Fade color:", "", PARAM_FADE_COLOR_DEFAULT);
    CKB_PARAM_LONG(PARAM_FADE_TIME_STRING, "Fade time:", "ms",
                    PARAM_FADE_TIME_DEFAULT, PARAM_FADE_TIME_MIN, PARAM_FADE_TIME_MAX);
    CKB_PARAM_LONG(PARAM_MAX_FADE_TIME_STRING, "Max fade time:", "s",
                    PARAM_MAX_FADE_TIME_DEFAULT, PARAM_MAX_FADE_TIME_MIN, PARAM_MAX_FADE_TIME_MAX);
    CKB_PARAM_LONG(PARAM_PRESSES_TO_MAX_INTENSITY_STRING, "Presses to max:", "key presses",
                    PARAM_PRESSES_TO_MAX_INTENSITY_DEFAULT, PARAM_PRESSES_TO_MAX_INTENSITY_MIN, PARAM_PRESSES_TO_MAX_INTENSITY_MAX);

    CKB_KPMODE(CKB_KP_NAME);
    CKB_TIMEMODE(CKB_TIME_ABSOLUTE);
    CKB_REPEAT(FALSE);
    CKB_LIVEPARAMS(FALSE);

    CKB_PRESET_START("Individual Keys");
    CKB_PRESET_PARAM(PARAM_FADE_TIME_STRING, TO_STRING(PARAM_FADE_TIME_DEFAULT));
    CKB_PRESET_PARAM(PARAM_MAX_FADE_TIME_STRING, TO_STRING(PARAM_MAX_FADE_TIME_DEFAULT));
    CKB_PRESET_PARAM("trigger", "0");
    CKB_PRESET_PARAM("kptrigger", "1");
    CKB_PRESET_END;
}


void ckb_init(ckb_runctx* context)
{
    gKeyStateInfo = malloc(context->keycount * sizeof(*gKeyStateInfo));
    unsigned int keyIdx;
    for (keyIdx = 0; keyIdx < context->keycount; keyIdx++)
    {
        gKeyStateInfo[keyIdx].xPos = context->keys[keyIdx].x;
        gKeyStateInfo[keyIdx].yPos = context->keys[keyIdx].y;
        gKeyStateInfo[keyIdx].isPressed = false;
    }
}


void ckb_parameter(ckb_runctx* context, const char* name, const char* value)
{
    context=context;
    name=name;
    value=value;

    CKB_PARSE_AGRADIENT(PARAM_FADE_COLOR_STRING, &gParamFadeGradient){};
    CKB_PARSE_LONG(PARAM_FADE_TIME_STRING, &gParamFadeTime){};
    CKB_PARSE_LONG(PARAM_MAX_FADE_TIME_STRING, &gParamMaxFadeTime){};
    CKB_PARSE_DOUBLE(PARAM_PRESSES_TO_MAX_INTENSITY_STRING, &gParamPressesToMaxIntensity){};
}


void ckb_keypress(ckb_runctx* context, ckb_key* key, int x, int y, int state)
{
    x=x; y=y;
    int keyIdx = key - context->keys;
    if (state)
    {
        gKeyStateInfo[keyIdx].isPressed = true;
        gKeyStateInfo[keyIdx].fadeTime += gParamFadeTime;
        gKeyStateInfo[keyIdx].fadeTime = CLAMP(gKeyStateInfo[keyIdx].fadeTime, 0, SEC_TO_MS(gParamMaxFadeTime));
    }
    else
    {
        gKeyStateInfo[keyIdx].isPressed = false;
    }
}


void ckb_start(ckb_runctx* context, int state)
{
    context=context;
    state=state;
}


void ckb_time(ckb_runctx* context, double delta)
{
    unsigned int keyIdx;
    for (keyIdx = 0; keyIdx < context->keycount; keyIdx++)
    {
        KEY_STATE_INFO_T * key = &gKeyStateInfo[keyIdx];
        if (key->fadeTime > 0 && !key->isPressed)
        {
            key->fadeTime -= (long)SEC_TO_MS(delta);
            debugPrint("idx=%d; delta=%f; fadeTime = %ld", keyIdx, delta, key->fadeTime);
        }
        key->fadeTime = CLAMP(key->fadeTime, 0, SEC_TO_MS(gParamMaxFadeTime));
    }
}


int ckb_frame(ckb_runctx* context)
{
    CKB_KEYCLEAR(context);

    unsigned int keyIdx;
    for (keyIdx = 0; keyIdx < context->keycount; keyIdx++)
    {
        KEY_STATE_INFO_T * key = &gKeyStateInfo[keyIdx];
        if (key->fadeTime > 0)
        {
            long maxIntensityTime = gParamPressesToMaxIntensity * gParamFadeTime;
            float intensity = CLAMP(100.f * (maxIntensityTime - key->fadeTime) / maxIntensityTime, 0, 100);
            debugPrint("intensity = %f; maxIntensityTime=%ld", intensity, maxIntensityTime);
            float a, r, g, b;
            ckb_grad_color(&a, &r, &g, &b, &gParamFadeGradient, intensity);
            ckb_alpha_blend(&context->keys[keyIdx], a, r, g, b);
        }
    }

    return 0;
}
