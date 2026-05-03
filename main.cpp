#include <stdio.h>
#include <math.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "ofxsImageEffect.h"
#include "ofxsMultiThread.h"
#include "ofxsInteract.h"
#include "ofxsLog.h"
#include "ofxsCore.h"

// ==============================================================================
// MugInteract: draws and handles the rectangle selection UI
// ==============================================================================
class MugInteract : public OFX::OverlayInteract {
protected:
    OFX::Double2DParam* _rectCenter;
    OFX::DoubleParam* _rectWidth;
    OFX::DoubleParam* _rectHeight;

    bool _isDragging;

public:
    MugInteract(OfxInteractHandle handle, OFX::ImageEffect* effect)
        : OFX::OverlayInteract(handle), _isDragging(false)
    {
        _rectCenter = effect->fetchDouble2DParam("rectCenter");
        _rectWidth = effect->fetchDoubleParam("rectWidth");
        _rectHeight = effect->fetchDoubleParam("rectHeight");
    }

    virtual bool draw(const OFX::DrawArgs& args) override {
        if (!_rectCenter || !_rectWidth || !_rectHeight) return false;

        double cx, cy, w, h;
        _rectCenter->getValueAtTime(args.time, cx, cy);
        w = _rectWidth->getValueAtTime(args.time);
        h = _rectHeight->getValueAtTime(args.time);

        // Calculate size in pixels based on the view
        // Depending on the coordinate system, we might need to adjust for aspect ratio or size.
        // Assuming canonical coordinates [0..1] mapped to pixel space or just using coordinates directly.
        // For simplicity, let's draw in canonical coordinates.
        
        OfxPointD pScale = args.pixelScale;
        double hw = w * 0.5;
        double hh = h * 0.5;

        glPushMatrix();

        // If dragging, draw in a different color (e.g., Yellow), otherwise White
        if (_isDragging) {
            glColor3f(1.0f, 1.0f, 0.0f);
        } else {
            glColor3f(1.0f, 1.0f, 1.0f);
        }

        glBegin(GL_LINE_LOOP);
        glVertex2d(cx - hw, cy - hh);
        glVertex2d(cx - hw, cy + hh);
        glVertex2d(cx + hw, cy + hh);
        glVertex2d(cx + hw, cy - hh);
        glEnd();

        // Draw a small crosshair at the center
        double crossSize = 10.0 * pScale.x;
        glBegin(GL_LINES);
        glVertex2d(cx - crossSize, cy);
        glVertex2d(cx + crossSize, cy);
        glVertex2d(cx, cy - crossSize);
        glVertex2d(cx, cy + crossSize);
        glEnd();

        glPopMatrix();

        return true;
    }

    virtual bool penDown(const OFX::PenArgs& args) override {
        if (!_rectCenter || !_rectWidth || !_rectHeight) return false;

        double cx, cy, w, h;
        _rectCenter->getValue(cx, cy);
        w = _rectWidth->getValue();
        h = _rectHeight->getValue();

        double hw = w * 0.5;
        double hh = h * 0.5;

        // Check if pen is inside the rectangle
        double px = args.penPosition.x;
        double py = args.penPosition.y;

        if (px >= cx - hw && px <= cx + hw && py >= cy - hh && py <= cy + hh) {
            _isDragging = true;
            _effect->redrawOverlays();
            return true;
        }

        return false;
    }

    virtual bool penMotion(const OFX::PenArgs& args) override {
        if (_isDragging) {
            // Move center
            _rectCenter->setValue(args.penPosition.x, args.penPosition.y);
            _effect->redrawOverlays();
            return true;
        }
        return false;
    }

    virtual bool penUp(const OFX::PenArgs& args) override {
        if (_isDragging) {
            _isDragging = false;
            _effect->redrawOverlays();
            return true;
        }
        return false;
    }
};

// Overlay descriptor subclass
class MugOverlayDescriptor : public OFX::DefaultEffectOverlayDescriptor<MugOverlayDescriptor, MugInteract> {};


// ==============================================================================
// MugPlugin: The Image Effect
// ==============================================================================
class MugPlugin : public OFX::ImageEffect {
protected:
    OFX::Clip *dstClip_;
    OFX::Clip *srcClip_;

    OFX::Double2DParam *rectCenter_;
    OFX::DoubleParam *rectWidth_;
    OFX::DoubleParam *rectHeight_;

public:
    MugPlugin(OfxImageEffectHandle handle)
        : OFX::ImageEffect(handle)
    {
        dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
        srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);

        rectCenter_ = fetchDouble2DParam("rectCenter");
        rectWidth_ = fetchDoubleParam("rectWidth");
        rectHeight_ = fetchDoubleParam("rectHeight");
    }

    virtual void render(const OFX::RenderArguments &args) override {
        // We do a dummy pass-through or simple render here if needed.
        // But since this plugin is primarily for UI testing, we don't need a complex render.
        // Let's just do identity or nothing.
    }

    virtual bool isIdentity(const OFX::IsIdentityArguments &args, OFX::Clip * &identityClip, double &identityTime) override {
        // Act as a pass-through filter so we can see the image
        identityClip = srcClip_;
        identityTime = args.time;
        return true;
    }
};

// ==============================================================================
// MugPluginFactory
// ==============================================================================
using namespace OFX;

class MugPluginFactory : public OFX::PluginFactoryHelper<MugPluginFactory> {
public:
    MugPluginFactory(const std::string& id, unsigned int verMaj, unsigned int verMin)
        : OFX::PluginFactoryHelper<MugPluginFactory>(id, verMaj, verMin) {}

    virtual void describe(OFX::ImageEffectDescriptor& desc) override {
        desc.setLabels("Mug Min Plugin 8", "Mug Min Plugin 8", "Mug Min Plugin 8");
        desc.setPluginGrouping("MugLab");

        desc.addSupportedContext(eContextFilter);
        desc.addSupportedContext(eContextGeneral);

        desc.addSupportedBitDepth(eBitDepthFloat);
        desc.addSupportedBitDepth(eBitDepthUByte);
        desc.addSupportedBitDepth(eBitDepthUShort);

        desc.setSingleInstance(false);
        desc.setHostFrameThreading(false);
        desc.setSupportsMultiResolution(true);
        desc.setSupportsTiles(true);
        desc.setTemporalClipAccess(false);
        desc.setRenderTwiceAlways(false);
        desc.setSupportsMultipleClipPARs(false);

        // Attach our custom interact
        desc.setOverlayInteractDescriptor(new MugOverlayDescriptor());
    }

    virtual void describeInContext(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context) override {
        // Clips
        ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
        srcClip->addSupportedComponent(ePixelComponentRGBA);
        srcClip->addSupportedComponent(ePixelComponentAlpha);
        srcClip->setTemporalClipAccess(false);
        srcClip->setSupportsTiles(true);
        srcClip->setIsMask(false);

        ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
        dstClip->addSupportedComponent(ePixelComponentRGBA);
        dstClip->addSupportedComponent(ePixelComponentAlpha);
        dstClip->setSupportsTiles(true);

        // Parameters
        PageParamDescriptor *page = desc.definePageParam("Controls");

        Double2DParamDescriptor *centerParam = desc.defineDouble2DParam("rectCenter");
        centerParam->setLabels("Center Position", "Center Position", "Center Position");
        centerParam->setDefault(0.5, 0.5);
        page->addChild(*centerParam);

        DoubleParamDescriptor *widthParam = desc.defineDoubleParam("rectWidth");
        widthParam->setLabels("Width", "Width", "Width");
        widthParam->setDefault(0.5);
        widthParam->setRange(0.0, 1.0);
        page->addChild(*widthParam);

        DoubleParamDescriptor *heightParam = desc.defineDoubleParam("rectHeight");
        heightParam->setLabels("Height", "Height", "Height");
        heightParam->setDefault(0.5);
        heightParam->setRange(0.0, 1.0);
        page->addChild(*heightParam);
    }

    virtual OFX::ImageEffect* createInstance(OfxImageEffectHandle handle, OFX::ContextEnum context) override {
        return new MugPlugin(handle);
    }
};

// ==============================================================================
// Plugin Registration
// ==============================================================================
namespace OFX {
    namespace Plugin {
        void getPluginIDs(OFX::PluginFactoryArray &ids) {
            static MugPluginFactory p("com.muglab.minplugin8", 1, 0);
            ids.push_back(&p);
        }
    }
}