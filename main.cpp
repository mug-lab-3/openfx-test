#include "ofxCore.h"
#include "ofxImageEffect.h"
#include "ofxProperty.h"
#include "ofxParam.h"
#include <string.h>

#if defined(_WIN32)
    #define EXPORT __declspec(dllexport)
#else
    #define EXPORT __attribute__((visibility("default")))
#endif

const OfxHost *gHost = nullptr;
const OfxPropertySuiteV1 *gPropSuite = nullptr;
const OfxImageEffectSuiteV1 *gEffectSuite = nullptr;
const OfxParameterSuiteV1 *gParamSuite = nullptr;

static OfxStatus mainEntryPoint(const char *action, const void *handle, OfxPropertySetHandle inArgs, OfxPropertySetHandle outArgs) {
    
    if (strcmp(action, "OfxActionLoad") == 0) {
        if (gHost) {
            gPropSuite = (const OfxPropertySuiteV1 *)gHost->fetchSuite(gHost->host, "OfxPropertySuite", 1);
            gEffectSuite = (const OfxImageEffectSuiteV1 *)gHost->fetchSuite(gHost->host, "OfxImageEffectSuite", 1);
            gParamSuite = (const OfxParameterSuiteV1 *)gHost->fetchSuite(gHost->host, "OfxParameterSuite", 1);
        }
        return kOfxStatOK;
    }

    if (strcmp(action, "OfxActionDescribe") == 0) {
        OfxImageEffectHandle effect = (OfxImageEffectHandle)handle;
        OfxPropertySetHandle effectProps = nullptr;
        
        if (gEffectSuite && gPropSuite) {
            gEffectSuite->getPropertySet(effect, &effectProps);
            if (effectProps) {
                // お名前を「8」に更新！
                gPropSuite->propSetString(effectProps, "OfxPropLabel", 0, "Mug Min Plugin 8");
                gPropSuite->propSetString(effectProps, "OfxImageEffectPluginPropGrouping", 0, "MugLab");
                gPropSuite->propSetString(effectProps, "OfxImageEffectPropSupportedContexts", 0, "OfxImageEffectContextFilter");
                gPropSuite->propSetString(effectProps, "OfxImageEffectPropSupportedContexts", 1, "OfxImageEffectContextGeneral");
                gPropSuite->propSetString(effectProps, "OfxImageEffectPropSupportedPixelDepths", 0, "OfxBitDepthFloat");
            }
        }
        return kOfxStatOK;
    }

    if (strcmp(action, "OfxImageEffectActionDescribeInContext") == 0) {
        OfxImageEffectHandle effect = (OfxImageEffectHandle)handle;
        OfxPropertySetHandle clipProps = nullptr;
        
        if (gEffectSuite && gPropSuite) {
            gEffectSuite->clipDefine(effect, "Output", &clipProps);
            gPropSuite->propSetString(clipProps, "OfxImageClipPropSupportedComponents", 0, "OfxImageComponentRGBA");
            gPropSuite->propSetString(clipProps, "OfxImageClipPropSupportedComponents", 1, "OfxImageComponentAlpha");

            gEffectSuite->clipDefine(effect, "Source", &clipProps);
            gPropSuite->propSetString(clipProps, "OfxImageClipPropSupportedComponents", 0, "OfxImageComponentRGBA");
            gPropSuite->propSetString(clipProps, "OfxImageClipPropSupportedComponents", 1, "OfxImageComponentAlpha");

            OfxParamSetHandle paramSet = nullptr;
            gEffectSuite->getParamSet(effect, &paramSet);
            
            if (paramSet && gParamSuite) {
                OfxPropertySetHandle paramProps = nullptr;
                
                // 1. 矩形の中心位置 (2D座標: X, Y)
                gParamSuite->paramDefine(paramSet, "OfxParamTypeDouble2D", "rectCenter", &paramProps);
                gPropSuite->propSetString(paramProps, "OfxPropLabel", 0, "Center Position");
                // XとYの初期値を画面のど真ん中（0.5, 0.5）に設定します
                gPropSuite->propSetDouble(paramProps, "OfxParamPropDefault", 0, 0.5); 
                gPropSuite->propSetDouble(paramProps, "OfxParamPropDefault", 1, 0.5); 
                
                // 2. 矩形の幅 (Width)
                gParamSuite->paramDefine(paramSet, "OfxParamTypeDouble", "rectWidth", &paramProps);
                gPropSuite->propSetString(paramProps, "OfxPropLabel", 0, "Width");
                gPropSuite->propSetDouble(paramProps, "OfxParamPropDefault", 0, 0.5);
                gPropSuite->propSetDouble(paramProps, "OfxParamPropMin", 0, 0.0);
                gPropSuite->propSetDouble(paramProps, "OfxParamPropMax", 0, 1.0);

                // 3. 矩形の高さ (Height)
                gParamSuite->paramDefine(paramSet, "OfxParamTypeDouble", "rectHeight", &paramProps);
                gPropSuite->propSetString(paramProps, "OfxPropLabel", 0, "Height");
                gPropSuite->propSetDouble(paramProps, "OfxParamPropDefault", 0, 0.5);
                gPropSuite->propSetDouble(paramProps, "OfxParamPropMin", 0, 0.0);
                gPropSuite->propSetDouble(paramProps, "OfxParamPropMax", 0, 1.0);
            }
        }
        return kOfxStatOK;
    }

    if (strcmp(action, "OfxImageEffectActionRender") == 0 || 
        strcmp(action, "OfxActionCreateInstance") == 0) {
        return kOfxStatOK;
    }

    return kOfxStatReplyDefault;
}

static void setHost(OfxHost *host) { gHost = host; }

static OfxPlugin pluginStruct = {
    kOfxImageEffectPluginApi, 1,                        
    "com.muglab.minplugin8", 
    1, 0, setHost, mainEntryPoint            
};

extern "C" {
    EXPORT int OfxGetNumberOfPlugins(void) { return 1; }
    EXPORT OfxPlugin *OfxGetPlugin(int nth) {
        if (nth == 0) return &pluginStruct;
        return nullptr;
    }
    EXPORT OfxStatus OfxSetHost(const OfxHost *host) {
        gHost = host;
        return kOfxStatOK;
    }
}