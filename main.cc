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

#include "ofxsCore.h"
#include "ofxsImageEffect.h"
#include "ofxsInteract.h"
#include "ofxsProcessing.h"

// ==============================================================================
// Image Processor: Multi-threaded pixel processing
// ==============================================================================
template <class PIX, int nComponents, int max>
class MugGreenScaler : public OFX::ImageProcessor {
   protected:
    OFX::Image* src_img_;

   public:
    MugGreenScaler(OFX::ImageEffect& instance) : OFX::ImageProcessor(instance), src_img_(nullptr) {
    }

    void setSrcImg(OFX::Image* v) {
        src_img_ = v;
    }

    virtual void multiThreadProcessImages(OfxRectI procWindow) override {
        for (int y = procWindow.y1; y < procWindow.y2; y++) {
            if (_effect.abort()) break;
            PIX* dstPix = (PIX*)_dstImg->getPixelAddress(procWindow.x1, y);
            PIX* srcPix = (PIX*)src_img_->getPixelAddress(procWindow.x1, y);

            for (int x = procWindow.x1; x < procWindow.x2; x++) {
                for (int c = 0; c < nComponents; c++) {
                    if (c == 1) {  // Green channel
                        dstPix[c] = (PIX)(srcPix[c] * 0.5);
                    } else {
                        dstPix[c] = srcPix[c];
                    }
                }
                dstPix += nComponents;
                srcPix += nComponents;
            }
        }
    }
};

// ==============================================================================
// MugInteract: draws and handles the rectangle selection UI
// ==============================================================================
class MugInteract : public OFX::OverlayInteract {
   protected:
    OFX::Double2DParam* rect_center_;
    OFX::DoubleParam* rect_width_;
    OFX::DoubleParam* rect_height_;
    OFX::Clip* src_clip_;

    enum DragMode {
        eModeNone,
        eModeCenter,
        eModeTopLeft,
        eModeTopRight,
        eModeBottomLeft,
        eModeBottomRight
    };
    DragMode drag_mode_;

   public:
    MugInteract(OfxInteractHandle handle, OFX::ImageEffect* effect)
        : OFX::OverlayInteract(handle), drag_mode_(eModeNone) {
        rect_center_ = effect->fetchDouble2DParam("rectCenter");
        rect_width_ = effect->fetchDoubleParam("rectWidth");
        rect_height_ = effect->fetchDoubleParam("rectHeight");
        src_clip_ = effect->fetchClip(kOfxImageEffectSimpleSourceClipName);
    }

    void getPixelRect(double time, double& cx, double& cy, double& w, double& h, OfxRectD& rod) {
        rod = src_clip_->getRegionOfDefinition(time);
        double rw = rod.x2 - rod.x1;
        double rh = rod.y2 - rod.y1;
        double ncx;
        double ncy;
        double nw;
        double nh;
        rect_center_->getValueAtTime(time, ncx, ncy);
        nw = rect_width_->getValueAtTime(time);
        nh = rect_height_->getValueAtTime(time);
        cx = rod.x1 + ncx * rw;
        cy = rod.y1 + ncy * rh;
        w = nw * rw;
        h = nh * rh;
    }

    virtual bool draw(const OFX::DrawArgs& args) override {
        double cx;
        double cy;
        double w;
        double h;
        OfxRectD rod;
        getPixelRect(args.time, cx, cy, w, h, rod);
        double hw = w * 0.5;
        double hh = h * 0.5;
        double x1 = cx - hw;
        double x2 = cx + hw;
        double y1 = cy - hh;
        double y2 = cy + hh;

        glPushMatrix();
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2d(x1, y1);
        glVertex2d(x1, y2);
        glVertex2d(x2, y2);
        glVertex2d(x2, y1);
        glEnd();

        double handleSize = 8.0 * args.pixelScale.x;
        auto drawHandle = [&](double x, double y, bool active) {
            if (active)
                glColor3f(1.0f, 1.0f, 0.0f);
            else
                glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_QUADS);
            glVertex2d(x - handleSize, y - handleSize);
            glVertex2d(x - handleSize, y + handleSize);
            glVertex2d(x + handleSize, y + handleSize);
            glVertex2d(x + handleSize, y - handleSize);
            glEnd();
            glColor3f(0.0f, 0.0f, 0.0f);
            glBegin(GL_LINE_LOOP);
            glVertex2d(x - handleSize, y - handleSize);
            glVertex2d(x - handleSize, y + handleSize);
            glVertex2d(x + handleSize, y + handleSize);
            glVertex2d(x + handleSize, y - handleSize);
            glEnd();
        };

        drawHandle(x1, y1, drag_mode_ == eModeBottomLeft);
        drawHandle(x1, y2, drag_mode_ == eModeTopLeft);
        drawHandle(x2, y2, drag_mode_ == eModeTopRight);
        drawHandle(x2, y1, drag_mode_ == eModeBottomRight);

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
        double cx;
        double cy;
        double w;
        double h;
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

        if (abs(px - x1) < tol && abs(py - y1) < tol)
            drag_mode_ = eModeBottomLeft;
        else if (abs(px - x1) < tol && abs(py - y2) < tol)
            drag_mode_ = eModeTopLeft;
        else if (abs(px - x2) < tol && abs(py - y2) < tol)
            drag_mode_ = eModeTopRight;
        else if (abs(px - x2) < tol && abs(py - y1) < tol)
            drag_mode_ = eModeBottomRight;
        else if (px >= x1 && px <= x2 && py >= y1 && py <= y2)
            drag_mode_ = eModeCenter;
        else
            drag_mode_ = eModeNone;

        if (drag_mode_ != eModeNone) {
            _effect->redrawOverlays();
            return true;
        }
        return false;
    }

    virtual bool penMotion(const OFX::PenArgs& args) override {
        if (drag_mode_ == eModeNone) return false;
        OfxRectD rod = src_clip_->getRegionOfDefinition(args.time);
        double rw = rod.x2 - rod.x1;
        double rh = rod.y2 - rod.y1;
        if (rw <= 0 || rh <= 0) return false;
        double ncx;
        double ncy;
        double nw;
        double nh;
        rect_center_->getValue(ncx, ncy);
        nw = rect_width_->getValue();
        nh = rect_height_->getValue();
        double npx = (args.penPosition.x - rod.x1) / rw;
        double npy = (args.penPosition.y - rod.y1) / rh;

        if (drag_mode_ == eModeCenter) {
            rect_center_->setValue(npx, npy);
        } else {
            double x1 = ncx - nw * 0.5;
            double x2 = ncx + nw * 0.5;
            double y1 = ncy - nh * 0.5;
            double y2 = ncy + nh * 0.5;
            if (drag_mode_ == eModeBottomLeft) {
                x1 = npx;
                y1 = npy;
            }
            if (drag_mode_ == eModeTopLeft) {
                x1 = npx;
                y2 = npy;
            }
            if (drag_mode_ == eModeTopRight) {
                x2 = npx;
                y2 = npy;
            }
            if (drag_mode_ == eModeBottomRight) {
                x2 = npx;
                y1 = npy;
            }
            rect_center_->setValue((x1 + x2) * 0.5, (y1 + y2) * 0.5);
            rect_width_->setValue(abs(x2 - x1));
            rect_height_->setValue(abs(y2 - y1));
        }
        _effect->redrawOverlays();
        return true;
    }

    virtual bool penUp(const OFX::PenArgs& args) override {
        if (drag_mode_ != eModeNone) {
            drag_mode_ = eModeNone;
            _effect->redrawOverlays();
            return true;
        }
        return false;
    }
};

class MugOverlayDescriptor : public OFX::DefaultEffectOverlayDescriptor<MugOverlayDescriptor, MugInteract> {};

// ==============================================================================
// MugPlugin: The Image Effect
// ==============================================================================
class MugPlugin : public OFX::ImageEffect {
   protected:
    OFX::Clip* dst_clip_;
    OFX::Clip* src_clip_;

   public:
    MugPlugin(OfxImageEffectHandle handle) : OFX::ImageEffect(handle) {
        dst_clip_ = fetchClip(kOfxImageEffectOutputClipName);
        src_clip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    }

    virtual void render(const OFX::RenderArguments& args) override {
        OFX::BitDepthEnum dstBitDepth = dst_clip_->getPixelDepth();
        OFX::PixelComponentEnum dstComponents = dst_clip_->getPixelComponents();

        std::unique_ptr<OFX::Image> dst(dst_clip_->fetchImage(args.time));
        std::unique_ptr<OFX::Image> src(src_clip_->fetchImage(args.time));

        if (dstComponents == OFX::ePixelComponentRGBA) {
            switch (dstBitDepth) {
                case OFX::eBitDepthUByte: {
                    MugGreenScaler<unsigned char, 4, 255> processor(*this);
                    processor.setDstImg(dst.get());
                    processor.setSrcImg(src.get());
                    processor.setRenderWindow(args.renderWindow);
                    processor.process();
                    break;
                }
                case OFX::eBitDepthUShort: {
                    MugGreenScaler<unsigned short, 4, 65535> processor(*this);
                    processor.setDstImg(dst.get());
                    processor.setSrcImg(src.get());
                    processor.setRenderWindow(args.renderWindow);
                    processor.process();
                    break;
                }
                case OFX::eBitDepthFloat: {
                    MugGreenScaler<float, 4, 1> processor(*this);
                    processor.setDstImg(dst.get());
                    processor.setSrcImg(src.get());
                    processor.setRenderWindow(args.renderWindow);
                    processor.process();
                    break;
                }
                default:
                    break;
            }
        }
    }

    virtual bool isIdentity(const OFX::IsIdentityArguments& args, OFX::Clip*& identityClip,
                            double& identityTime) override {
        return false;
    }
};

using namespace OFX;

class MugPluginFactory : public OFX::PluginFactoryHelper<MugPluginFactory> {
   public:
    MugPluginFactory(const std::string& id, unsigned int verMaj, unsigned int verMin)
        : OFX::PluginFactoryHelper<MugPluginFactory>(id, verMaj, verMin) {
    }

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

        PageParamDescriptor* page = desc.definePageParam("Controls");
        Double2DParamDescriptor* centerParam = desc.defineDouble2DParam("rectCenter");
        centerParam->setLabels("Center", "Center", "Center");
        centerParam->setDefault(0.5, 0.5);
        centerParam->setRange(0, 0, 1, 1);
        page->addChild(*centerParam);

        DoubleParamDescriptor* widthParam = desc.defineDoubleParam("rectWidth");
        widthParam->setLabels("Width", "Width", "Width");
        widthParam->setDefault(0.5);
        widthParam->setRange(0, 1);
        page->addChild(*widthParam);

        DoubleParamDescriptor* heightParam = desc.defineDoubleParam("rectHeight");
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
void getPluginIDs(OFX::PluginFactoryArray& ids) {
    static MugPluginFactory p("com.muglab.minplugin8", 1, 0);
    ids.push_back(&p);
}
}  // namespace Plugin
}  // namespace OFX