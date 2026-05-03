#include <stdio.h>
#include <math.h>
#include <algorithm>

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
    OFX::Clip* _srcClip;

    enum DragMode {
        eModeNone,
        eModeCenter,
        eModeTopLeft,
        eModeTopRight,
        eModeBottomLeft,
        eModeBottomRight
    };
    DragMode _dragMode;

public:
    MugInteract(OfxInteractHandle handle, OFX::ImageEffect* effect)
        : OFX::OverlayInteract(handle), _dragMode(eModeNone)
    {
        _rectCenter = effect->fetchDouble2DParam("rectCenter");
        _rectWidth = effect->fetchDoubleParam("rectWidth");
        _rectHeight = effect->fetchDoubleParam("rectHeight");
        _srcClip = effect->fetchClip(kOfxImageEffectSimpleSourceClipName);
    }

    // Map normalized (0..1) to pixel coordinates based on RoD
    void getPixelRect(double time, double &cx, double &cy, double &w, double &h, OfxRectD &rod) {
        rod = _srcClip->getRegionOfDefinition(time);
        double rw = rod.x2 - rod.x1;
        double rh = rod.y2 - rod.y1;

        double ncx, ncy, nw, nh;
        _rectCenter->getValueAtTime(time, ncx, ncy);
        nw = _rectWidth->getValueAtTime(time);
        nh = _rectHeight->getValueAtTime(time);

        cx = rod.x1 + ncx * rw;
        cy = rod.y1 + ncy * rh;
        w = nw * rw;
        h = nh * rh;
    }

    virtual bool draw(const OFX::DrawArgs& args) override {
        double cx, cy, w, h;
        OfxRectD rod;
        getPixelRect(args.time, cx, cy, w, h, rod);

        double hw = w * 0.5;
        double hh = h * 0.5;
        double x1 = cx - hw;
        double x2 = cx + hw;
        double y1 = cy - hh;
        double y2 = cy + hh;

        glPushMatrix();

        // Draw main rectangle
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2d(x1, y1);
        glVertex2d(x1, y2);
        glVertex2d(x2, y2);
        glVertex2d(x2, y1);
        glEnd();

        // Draw handles at corners
        double handleSize = 8.0 * args.pixelScale.x;
        auto drawHandle = [&](double x, double y, bool active) {
            if (active) glColor3f(1.0f, 1.0f, 0.0f);
            else glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_QUADS);
            glVertex2d(x - handleSize, y - handleSize);
            glVertex2d(x - handleSize, y + handleSize);
            glVertex2d(x + handleSize, y + handleSize);
            glVertex2d(x + handleSize, y - handleSize);
            glEnd();
            // outline
            glColor3f(0.0f, 0.0f, 0.0f);
            glBegin(GL_LINE_LOOP);
            glVertex2d(x - handleSize, y - handleSize);
            glVertex2d(x - handleSize, y + handleSize);
            glVertex2d(x + handleSize, y + handleSize);
            glVertex2d(x + handleSize, y - handleSize);
            glEnd();
        };

        drawHandle(x1, y1, _dragMode == eModeBottomLeft);
        drawHandle(x1, y2, _dragMode == eModeTopLeft);
        drawHandle(x2, y2, _dragMode == eModeTopRight);
        drawHandle(x2, y1, _dragMode == eModeBottomRight);

        // Draw center cross
        glColor3f(1.0f, 1.0f, 1.0f);
        double crossSize = 10.0 * args.pixelScale.x;
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
        double cx, cy, w, h;
        OfxRectD rod;
        getPixelRect(args.time, cx, cy, w, h, rod);

        double hw = w * 0.5;
        double hh = h * 0.5;
        double x1 = cx - hw;
        double x2 = cx + hw;
        double y1 = cy - hh;
        double y2 = cy + hh;

        double px = args.penPosition.x;
        double py = args.penPosition.y;
        double tol = 10.0 * args.pixelScale.x;

        if (abs(px - x1) < tol && abs(py - y1) < tol) _dragMode = eModeBottomLeft;
        else if (abs(px - x1) < tol && abs(py - y2) < tol) _dragMode = eModeTopLeft;
        else if (abs(px - x2) < tol && abs(py - y2) < tol) _dragMode = eModeTopRight;
        else if (abs(px - x2) < tol && abs(py - y1) < tol) _dragMode = eModeBottomRight;
        else if (px >= x1 && px <= x2 && py >= y1 && py <= y2) _dragMode = eModeCenter;
        else _dragMode = eModeNone;

        if (_dragMode != eModeNone) {
            _effect->redrawOverlays();
            return true;
        }
        return false;
    }

    virtual bool penMotion(const OFX::PenArgs& args) override {
        if (_dragMode == eModeNone) return false;

        OfxRectD rod = _srcClip->getRegionOfDefinition(args.time);
        double rw = rod.x2 - rod.x1;
        double rh = rod.y2 - rod.y1;
        if (rw <= 0 || rh <= 0) return false;

        double ncx, ncy, nw, nh;
        _rectCenter->getValue(ncx, ncy);
        nw = _rectWidth->getValue();
        nh = _rectHeight->getValue();

        double px = args.penPosition.x;
        double py = args.penPosition.y;

        // Convert pen to normalized
        double npx = (px - rod.x1) / rw;
        double npy = (py - rod.y1) / rh;

        if (_dragMode == eModeCenter) {
            _rectCenter->setValue(npx, npy);
        } else {
            // Resize logic
            double x1 = ncx - nw * 0.5;
            double x2 = ncx + nw * 0.5;
            double y1 = ncy - nh * 0.5;
            double y2 = ncy + nh * 0.5;

            if (_dragMode == eModeBottomLeft) { x1 = npx; y1 = npy; }
            if (_dragMode == eModeTopLeft) { x1 = npx; y2 = npy; }
            if (_dragMode == eModeTopRight) { x2 = npx; y2 = npy; }
            if (_dragMode == eModeBottomRight) { x2 = npx; y1 = npy; }

            double newNcx = (x1 + x2) * 0.5;
            double newNcy = (y1 + y2) * 0.5;
            double newNw = abs(x2 - x1);
            double newNh = abs(y2 - y1);

            _rectCenter->setValue(newNcx, newNcy);
            _rectWidth->setValue(newNw);
            _rectHeight->setValue(newNh);
        }

        _effect->redrawOverlays();
        return true;
    }

    virtual bool penUp(const OFX::PenArgs& args) override {
        if (_dragMode != eModeNone) {
            _dragMode = eModeNone;
            _effect->redrawOverlays();
            return true;
        }
        return false;
    }
};

class MugOverlayDescriptor : public OFX::DefaultEffectOverlayDescriptor<MugOverlayDescriptor, MugInteract> {};

class MugPlugin : public OFX::ImageEffect {
protected:
    OFX::Clip *dstClip_;
    OFX::Clip *srcClip_;
public:
    MugPlugin(OfxImageEffectHandle handle) : OFX::ImageEffect(handle) {
        dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
        srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    }
    virtual void render(const OFX::RenderArguments &args) override {}
    virtual bool isIdentity(const OFX::IsIdentityArguments &args, OFX::Clip * &identityClip, double &identityTime) override {
        identityClip = srcClip_;
        identityTime = args.time;
        return true;
    }
};

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
        desc.setOverlayInteractDescriptor(new MugOverlayDescriptor());
    }

    virtual void describeInContext(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context) override {
        desc.defineClip(kOfxImageEffectSimpleSourceClipName)->addSupportedComponent(ePixelComponentRGBA);
        desc.defineClip(kOfxImageEffectOutputClipName)->addSupportedComponent(ePixelComponentRGBA);

        PageParamDescriptor *page = desc.definePageParam("Controls");

        Double2DParamDescriptor *centerParam = desc.defineDouble2DParam("rectCenter");
        centerParam->setLabels("Center", "Center", "Center");
        centerParam->setDefault(0.5, 0.5);
        centerParam->setRange(0, 0, 1, 1);
        page->addChild(*centerParam);

        DoubleParamDescriptor *widthParam = desc.defineDoubleParam("rectWidth");
        widthParam->setLabels("Width", "Width", "Width");
        widthParam->setDefault(0.5);
        widthParam->setRange(0, 1);
        page->addChild(*widthParam);

        DoubleParamDescriptor *heightParam = desc.defineDoubleParam("rectHeight");
        heightParam->setLabels("Height", "Height", "Height");
        heightParam->setDefault(0.5);
        heightParam->setRange(0, 1);
        page->addChild(*heightParam);
    }

    virtual OFX::ImageEffect* createInstance(OfxImageEffectHandle handle, OFX::ContextEnum context) override {
        return new MugPlugin(handle);
    }
};

namespace OFX {
    namespace Plugin {
        void getPluginIDs(OFX::PluginFactoryArray &ids) {
            static MugPluginFactory p("com.muglab.minplugin8", 1, 0);
            ids.push_back(&p);
        }
    }
}