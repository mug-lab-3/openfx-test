#include <cmath>
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

#include <blend2d.h>

#include <concepts>
#include <cstdint>
#include <memory>
#include <span>

#include "ofxsCore.h"
#include "ofxsImageEffect.h"
#include "ofxsInteract.h"
#include "ofxsProcessing.h"



// --- Unified Performance Engine: 1-Pass Processing ---
template <typename PIX, int nComponents, int maxVal>
class MugUnifiedProcessor : public OFX::ImageProcessor {
    OFX::Image* _srcImg;
    BLImage& _blImg;
    OfxRectI _bounds;
    OfxRectI _drawWindow;
    int _height;

   public:
    MugUnifiedProcessor(OFX::ImageEffect& effect, OFX::Image* src, BLImage& bl, OfxRectI bounds, OfxRectI drawWin)
        : OFX::ImageProcessor(effect), _srcImg(src), _blImg(bl), _bounds(bounds), _drawWindow(drawWin) {
        _height = _bounds.y2 - _bounds.y1;
    }

    void multiThreadProcessImages(OfxRectI window) override {
        BLImageData blData;
        _blImg.get_data(&blData);

        for (int y = window.y1; y < window.y2; y++) {
            if (_effect.abort()) break;

            PIX* dst_p = reinterpret_cast<PIX*>(_dstImg->getPixelAddress(window.x1, y));
            const PIX* src_p = reinterpret_cast<const PIX*>(_srcImg->getPixelAddress(window.x1, y));

            bool rowInROI = (y >= _drawWindow.y1 && y < _drawWindow.y2);
            int roi_x1 = std::clamp(_drawWindow.x1, window.x1, window.x2);
            int roi_x2 = std::clamp(_drawWindow.x2, window.x1, window.x2);

            int x = window.x1;

            // Segment 1: Left of ROI
            for (; x < roi_x1; x++) {
                dst_p[0] = src_p[0];
                if constexpr (std::is_floating_point_v<PIX>) {
                    dst_p[1] = src_p[1] * 0.5f;
                } else {
                    dst_p[1] = src_p[1] >> 1;
                }
                dst_p[2] = src_p[2];
                dst_p[3] = src_p[3];
                dst_p += 4;
                src_p += 4;
            }

            // Segment 2: Inside ROI
            if (rowInROI && roi_x1 < roi_x2) {
                int bl_y = (_height - 1) - (y - _bounds.y1);
                uint32_t* bl_line =
                    reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(blData.pixel_data) + (bl_y * blData.stride));

                for (; x < roi_x2; x++) {
                    PIX g;
                    if constexpr (std::is_floating_point_v<PIX>) {
                        g = src_p[1] * 0.5f;
                    } else {
                        g = src_p[1] >> 1;
                    }
                    PIX r = src_p[0], b = src_p[2], a = src_p[3];

                    uint32_t p = bl_line[x - _bounds.x1];
                    uint32_t sa_8 = (p >> 24) & 0xFF;
                    if (sa_8 > 0) {
                        if constexpr (std::is_same_v<PIX, float>) {
                            constexpr float inv255 = 1.0f / 255.0f;
                            float sa = sa_8 * inv255;
                            float inv_sa = 1.0f - sa;
                            if (sa_8 == 255) {
                                r = ((p >> 16) & 0xFF) * inv255;
                                g = ((p >> 8) & 0xFF) * inv255;
                                b = (p & 0xFF) * inv255;
                                a = 1.0f;
                            } else {
                                r = (((p >> 16) & 0xFF) * inv255) + (r * inv_sa);
                                g = (((p >> 8) & 0xFF) * inv255) + (g * inv_sa);
                                b = ((p & 0xFF) * inv255) + (b * inv_sa);
                                a = sa + (a * inv_sa);
                            }
                        } else {
                            uint32_t sa = (maxVal == 255) ? sa_8 : (sa_8 * 257);
                            uint32_t inv_sa = maxVal - sa;
                            uint32_t sr = ((p >> 16) & 0xFF), sg = ((p >> 8) & 0xFF), sb = (p & 0xFF);
                            if (maxVal != 255) {
                                sr *= 257;
                                sg *= 257;
                                sb *= 257;
                            }
                            r = (PIX)(sr + (r * inv_sa + (maxVal / 2)) / maxVal);
                            g = (PIX)(sg + (g * inv_sa + (maxVal / 2)) / maxVal);
                            b = (PIX)(sb + (b * inv_sa + (maxVal / 2)) / maxVal);
                            a = (PIX)(sa + (a * inv_sa + (maxVal / 2)) / maxVal);
                        }
                    }
                    dst_p[0] = r;
                    dst_p[1] = g;
                    dst_p[2] = b;
                    dst_p[3] = a;
                    dst_p += 4;
                    src_p += 4;
                }
            }

            // Segment 3: Right of ROI
            for (; x < window.x2; x++) {
                dst_p[0] = src_p[0];
                if constexpr (std::is_floating_point_v<PIX>) {
                    dst_p[1] = src_p[1] * 0.5f;
                } else {
                    dst_p[1] = src_p[1] >> 1;
                }
                dst_p[2] = src_p[2];
                dst_p[3] = src_p[3];
                dst_p += 4;
                src_p += 4;
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

    enum DragMode : std::uint8_t {
        eModeNone,
        eModeCenter,
        eModeTopLeft,
        eModeTopRight,
        eModeBottomLeft,
        eModeBottomRight
    };
    DragMode drag_mode_{eModeNone};

   public:
    MugInteract(OfxInteractHandle handle, OFX::ImageEffect* effect) : OFX::OverlayInteract(handle) {
        rect_center_ = effect->fetchDouble2DParam("rectCenter");
        rect_width_ = effect->fetchDoubleParam("rectWidth");
        rect_height_ = effect->fetchDoubleParam("rectHeight");
        src_clip_ = effect->fetchClip(kOfxImageEffectSimpleSourceClipName);
    }

    struct PixelRect {
        double cx_;
        double cy_;
        double w_;
        double h_;
        OfxRectD rod_;
    };

    auto getPixelRect(double time) -> PixelRect {
        PixelRect pr;
        pr.rod_ = src_clip_->getRegionOfDefinition(time);
        double rw = pr.rod_.x2 - pr.rod_.x1;
        double rh = pr.rod_.y2 - pr.rod_.y1;
        double ncx;
        double ncy;
        double nw;
        double nh;
        rect_center_->getValueAtTime(time, ncx, ncy);
        nw = rect_width_->getValueAtTime(time);
        nh = rect_height_->getValueAtTime(time);
        pr.cx_ = pr.rod_.x1 + (ncx * rw);
        pr.cy_ = pr.rod_.y1 + (ncy * rh);
        pr.w_ = nw * rw;
        pr.h_ = nh * rh;
        return pr;
    }

    auto draw(const OFX::DrawArgs& args) -> bool override {
        PixelRect pr = getPixelRect(args.time);
        double hw = pr.w_ * 0.5;
        double hh = pr.h_ * 0.5;
        double x1 = pr.cx_ - hw;
        double x2 = pr.cx_ + hw;
        double y1 = pr.cy_ - hh;
        double y2 = pr.cy_ + hh;

        glPushMatrix();
        glColor3f(1.0F, 1.0F, 1.0F);
        glBegin(GL_LINE_LOOP);
        glVertex2d(x1, y1);
        glVertex2d(x1, y2);
        glVertex2d(x2, y2);
        glVertex2d(x2, y1);
        glEnd();

        double handleSize = 8.0 * args.pixelScale.x;
        auto drawHandle = [&](double x, double y, bool active) {
            if (active) {
                glColor3f(1.0F, 1.0F, 0.0F);
            } else {
                glColor3f(1.0F, 1.0F, 1.0F);
            }
            glBegin(GL_QUADS);
            glVertex2d(x - handleSize, y - handleSize);
            glVertex2d(x - handleSize, y + handleSize);
            glVertex2d(x + handleSize, y + handleSize);
            glVertex2d(x + handleSize, y - handleSize);
            glEnd();
            glColor3f(0.0F, 0.0F, 0.0F);
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

        glColor3f(1.0F, 1.0F, 1.0F);
        double crossSize = 10.0 * args.pixelScale.x;
        glBegin(GL_LINES);
        glVertex2d(pr.cx_ - crossSize, pr.cy_);
        glVertex2d(pr.cx_ + crossSize, pr.cy_);
        glVertex2d(pr.cx_, pr.cy_ - crossSize);
        glVertex2d(pr.cx_, pr.cy_ + crossSize);
        glEnd();
        glPopMatrix();
        return true;
    }

    auto penDown(const OFX::PenArgs& args) -> bool override {
        PixelRect pr = getPixelRect(args.time);
        double hw = pr.w_ * 0.5;
        double hh = pr.h_ * 0.5;
        double x1 = pr.cx_ - hw;
        double x2 = pr.cx_ + hw;
        double y1 = pr.cy_ - hh;
        double y2 = pr.cy_ + hh;
        double px = args.penPosition.x;
        double py = args.penPosition.y;
        double tol = 10.0 * args.pixelScale.x;

        if (abs(px - x1) < tol && abs(py - y1) < tol) {
            drag_mode_ = eModeBottomLeft;
        } else if (abs(px - x1) < tol && abs(py - y2) < tol) {
            drag_mode_ = eModeTopLeft;
        } else if (abs(px - x2) < tol && abs(py - y2) < tol) {
            drag_mode_ = eModeTopRight;
        } else if (abs(px - x2) < tol && abs(py - y1) < tol) {
            drag_mode_ = eModeBottomRight;
        } else if (px >= x1 && px <= x2 && py >= y1 && py <= y2) {
            drag_mode_ = eModeCenter;
        } else {
            drag_mode_ = eModeNone;
        }

        if (drag_mode_ != eModeNone) {
            _effect->redrawOverlays();
            return true;
        }
        return false;
    }

    auto penMotion(const OFX::PenArgs& args) -> bool override {
        if (drag_mode_ == eModeNone) {
            return false;
        }
        OfxRectD rod = src_clip_->getRegionOfDefinition(args.time);
        double rw = rod.x2 - rod.x1;
        double rh = rod.y2 - rod.y1;
        if (rw <= 0 || rh <= 0) {
            return false;
        }
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
            double x1 = ncx - (nw * 0.5);
            double x2 = ncx + (nw * 0.5);
            double y1 = ncy - (nh * 0.5);
            double y2 = ncy + (nh * 0.5);
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

    auto penUp(const OFX::PenArgs& args) -> bool override {
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
    BLImage _cachedImg;

   public:
    explicit MugPlugin(OfxImageEffectHandle handle) : OFX::ImageEffect(handle) {
        dst_clip_ = fetchClip(kOfxImageEffectOutputClipName);
        src_clip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    }

    void render(const OFX::RenderArguments& args) override {
        OFX::BitDepthEnum dstBitDepth = dst_clip_->getPixelDepth();
        OFX::PixelComponentEnum dstComponents = dst_clip_->getPixelComponents();

        std::unique_ptr<OFX::Image> dst(dst_clip_->fetchImage(args.time));
        std::unique_ptr<OFX::Image> src(src_clip_->fetchImage(args.time));

        if (dstComponents == OFX::ePixelComponentRGBA && src) {
            OfxRectI bounds = dst->getBounds();
            int width = bounds.x2 - bounds.x1;
            int height = bounds.y2 - bounds.y1;

            if (width > 0 && height > 0) {
                // 1. Prepare Blend2D Canvas
                if (_cachedImg.width() != width || _cachedImg.height() != height) {
                    _cachedImg.create(width, height, BL_FORMAT_PRGB32);
                }
                BLContextCreateInfo createInfo;
                createInfo.flags = BL_CONTEXT_CREATE_NO_FLAGS;
                createInfo.thread_count = 1; // Single thread to avoid oversubscription with OFX
                BLContext ctx(_cachedImg, createInfo);
                ctx.set_comp_op(BL_COMP_OP_SRC_COPY);
                ctx.fill_all(BLRgba32(0x00000000));
                ctx.set_comp_op(BL_COMP_OP_SRC_OVER);

                // 2. Draw Vector Graphics
                double ncx, ncy, nw, nh;
                fetchDouble2DParam("rectCenter")->getValueAtTime(args.time, ncx, ncy);
                nw = fetchDoubleParam("rectWidth")->getValueAtTime(args.time);
                nh = fetchDoubleParam("rectHeight")->getValueAtTime(args.time);
                double cx = ncx * width, cy = (1.0 - ncy) * height;
                double w = nw * width, h = nh * height;

                ctx.set_fill_style(BLRgba32(0x800000FF));
                ctx.fill_round_rect(BLRoundRect(cx - (w * 0.5), cy - (h * 0.5), w, h, 20.0));
                ctx.set_fill_style(BLRgba32(0xFFFFFF00));
                ctx.fill_circle(cx, cy, w * 0.2);
                ctx.set_stroke_style(BLRgba32(0xFFFFFFFF));
                ctx.set_stroke_width(2.0);
                ctx.stroke_circle(cx, cy, w * 0.2);

                static BLFontFace face; static bool fontLoaded = false;
                if (!fontLoaded) {
                    if (face.create_from_file("C:/Windows/Fonts/arial.ttf") == BL_SUCCESS ||
                        face.create_from_file("C:/Windows/Fonts/msgothic.ttc") == BL_SUCCESS ||
                        face.create_from_file("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf") == BL_SUCCESS) fontLoaded = true;
                }
                if (fontLoaded) {
                    BLFont font; font.create_from_face(face, 24.0f);
                    ctx.set_fill_style(BLRgba32(0xFFFFFFFF));
                    ctx.fill_utf8_text(BLPoint(cx - (w * 0.4), cy + (h * 0.4)), font, "Blend2D Hybrid Vector Text");
                }
                ctx.end();

                // 3. ROI Calculation (Map Blend2D to OFX)
                double padding = 20.0;
                int roi_x1 = static_cast<int>(std::floor(cx - (w * 0.5) - padding));
                int roi_y1 = static_cast<int>(std::floor(cy - (h * 0.5) - padding));
                int roi_x2 = static_cast<int>(std::ceil(cx + (w * 0.5) + padding));
                int roi_y2 = static_cast<int>(std::ceil(cy + (h * 0.5) + padding));

                OfxRectI drawWindow;
                drawWindow.x1 = bounds.x1 + roi_x1;
                drawWindow.x2 = bounds.x1 + roi_x2;
                drawWindow.y1 = bounds.y2 - roi_y2;
                drawWindow.y2 = bounds.y2 - roi_y1;

                // 4. Unified Processing Pass
                if (dstBitDepth == OFX::eBitDepthUByte) {
                    MugUnifiedProcessor<uint8_t, 4, 255> processor(*this, src.get(), _cachedImg, bounds, drawWindow);
                    processor.setDstImg(dst.get());
                    processor.setRenderWindow(args.renderWindow);
                    processor.process();
                } else if (dstBitDepth == OFX::eBitDepthUShort) {
                    MugUnifiedProcessor<uint16_t, 4, 65535> processor(*this, src.get(), _cachedImg, bounds, drawWindow);
                    processor.setDstImg(dst.get());
                    processor.setRenderWindow(args.renderWindow);
                    processor.process();
                } else if (dstBitDepth == OFX::eBitDepthFloat) {
                    MugUnifiedProcessor<float, 4, 1> processor(*this, src.get(), _cachedImg, bounds, drawWindow);
                    processor.setDstImg(dst.get());
                    processor.setRenderWindow(args.renderWindow);
                    processor.process();
                }
            }
        }
    }

    auto isIdentity(const OFX::IsIdentityArguments& args, OFX::Clip*& identityClip, double& identityTime)
        -> bool override {
        return false;
    }
};

class MugPluginFactory : public OFX::PluginFactoryHelper<MugPluginFactory> {
   public:
    MugPluginFactory(const std::string& id, unsigned int verMaj, unsigned int verMin)
        : OFX::PluginFactoryHelper<MugPluginFactory>(id, verMaj, verMin) {
    }

    void describe(OFX::ImageEffectDescriptor& desc) override {
        desc.setLabels("Mug Plugin", "Mug Plugin", "Mug Plugin");
        desc.setPluginGrouping("MugLab");
        desc.addSupportedContext(OFX::eContextFilter);
        desc.addSupportedContext(OFX::eContextGeneral);
        desc.addSupportedBitDepth(OFX::eBitDepthFloat);
        desc.addSupportedBitDepth(OFX::eBitDepthUByte);
        desc.addSupportedBitDepth(OFX::eBitDepthUShort);
        desc.setOverlayInteractDescriptor(new MugOverlayDescriptor());
    }

    void describeInContext(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context) override {
        desc.defineClip(kOfxImageEffectSimpleSourceClipName)->addSupportedComponent(OFX::ePixelComponentRGBA);
        desc.defineClip(kOfxImageEffectOutputClipName)->addSupportedComponent(OFX::ePixelComponentRGBA);

        OFX::PageParamDescriptor* page = desc.definePageParam("Controls");
        OFX::Double2DParamDescriptor* centerParam = desc.defineDouble2DParam("rectCenter");
        centerParam->setLabels("Center", "Center", "Center");
        centerParam->setDefault(0.5, 0.5);
        centerParam->setRange(0, 0, 1, 1);
        page->addChild(*centerParam);

        OFX::DoubleParamDescriptor* widthParam = desc.defineDoubleParam("rectWidth");
        widthParam->setLabels("Width", "Width", "Width");
        widthParam->setDefault(0.5);
        widthParam->setRange(0, 1);
        page->addChild(*centerParam);
        page->addChild(*widthParam);

        OFX::DoubleParamDescriptor* heightParam = desc.defineDoubleParam("rectHeight");
        heightParam->setLabels("Height", "Height", "Height");
        heightParam->setDefault(0.5);
        heightParam->setRange(0, 1);
        page->addChild(*heightParam);
    }

    auto createInstance(OfxImageEffectHandle handle, OFX::ContextEnum context) -> OFX::ImageEffect* override {
        return new MugPlugin(handle);
    }
};

namespace OFX::Plugin {
void getPluginIDs(OFX::PluginFactoryArray& ids) {
    static MugPluginFactory p("com.muglab.minplugin8", 1, 0);
    ids.push_back(&p);
}
}  // namespace OFX::Plugin