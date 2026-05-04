#include <cmath>

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

// ==============================================================================
// Image Processor: Multi-threaded pixel processing
// ==============================================================================
// Concepts: Ensure PIX is a numeric type (integer or floating point)
template <typename T>
concept OfxPixel = std::integral<T> || std::floating_point<T>;

template <OfxPixel PIX, int nComponents, int max>
class MugGreenScaler : public OFX::ImageProcessor {
   protected:
    OFX::Image* src_img_{nullptr};

   public:
    explicit MugGreenScaler(OFX::ImageEffect& instance) : OFX::ImageProcessor(instance) {
    }

    void setSrcImg(OFX::Image* v) {
        src_img_ = v;
    }

    void multiThreadProcessImages(OfxRectI procWindow) override {
        const int width = procWindow.x2 - procWindow.x1;

        for (int y = procWindow.y1; y < procWindow.y2; y++) {
            if (_effect.abort()) {
                break;
            }

            // Use std::span for safer memory access
            std::span<PIX> dst_row(static_cast<PIX*>(_dstImg->getPixelAddress(procWindow.x1, y)), width * nComponents);
            std::span<const PIX> src_row(static_cast<const PIX*>(src_img_->getPixelAddress(procWindow.x1, y)),
                                         width * nComponents);

            for (int x = 0; x < width; ++x) {
                const int pixel_offset = x * nComponents;
                for (int c = 0; c < nComponents; ++c) {
                    const int idx = pixel_offset + c;
                    if (c == 1) {  // Green channel
                        dst_row[idx] = static_cast<PIX>(src_row[idx] * 0.5);
                    } else {
                        dst_row[idx] = src_row[idx];
                    }
                }
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
            constexpr int kMaxUByte = 255;
            constexpr int kMaxUShort = 65535;
            constexpr float kMaxFloat = 1.0F;

            switch (dstBitDepth) {
                case OFX::eBitDepthUByte: {
                    MugGreenScaler<unsigned char, 4, kMaxUByte> processor(*this);
                    processor.setDstImg(dst.get());
                    processor.setSrcImg(src.get());
                    processor.setRenderWindow(args.renderWindow);
                    processor.process();
                    break;
                }
                case OFX::eBitDepthUShort: {
                    MugGreenScaler<uint16_t, 4, kMaxUShort> processor(*this);
                    processor.setDstImg(dst.get());
                    processor.setSrcImg(src.get());
                    processor.setRenderWindow(args.renderWindow);
                    processor.process();
                    break;
                }
                case OFX::eBitDepthFloat: {
                    MugGreenScaler<float, 4, static_cast<int>(kMaxFloat)> processor(*this);
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

        // ==============================================================================
        // 2. Vector Processing (Blend2D) - Hybrid Sample
        // ==============================================================================
        if (dstBitDepth == OFX::eBitDepthUByte || dstBitDepth == OFX::eBitDepthFloat) {
            OfxRectI bounds = dst->getBounds();
            int width = bounds.x2 - bounds.x1;
            int height = bounds.y2 - bounds.y1;

            if (width > 0 && height > 0) {
                BLImage img(width, height, BL_FORMAT_PRGB32);
                BLContext ctx(img);

                // Clear temporary buffer (transparent)
                ctx.set_comp_op(BL_COMP_OP_SRC_COPY);
                ctx.fill_all(BLRgba32(0x00000000));
                ctx.set_comp_op(BL_COMP_OP_SRC_OVER);

                // --- Fetch Parameters ---
                double ncx, ncy, nw, nh;
                fetchDouble2DParam("rectCenter")->getValueAtTime(args.time, ncx, ncy);
                nw = fetchDoubleParam("rectWidth")->getValueAtTime(args.time);
                nh = fetchDoubleParam("rectHeight")->getValueAtTime(args.time);

                double cx = ncx * width;
                double cy = (1.0 - ncy) * height;
                double w = nw * width;
                double h = nh * height;

                // --- Draw Shapes ---
                ctx.set_fill_style(BLRgba32(0x800000FF));
                ctx.fill_round_rect(BLRoundRect(cx - (w * 0.5), cy - (h * 0.5), w, h, 20.0));

                ctx.set_fill_style(BLRgba32(0xFFFFFF00));
                ctx.fill_circle(cx, cy, w * 0.2);

                ctx.set_stroke_style(BLRgba32(0xFFFFFFFF));
                ctx.set_stroke_width(2.0);
                ctx.stroke_circle(cx, cy, w * 0.2);

                // --- Draw Text ---
                BLFontFace face;
                if (face.create_from_file("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf") ==
                        BL_SUCCESS ||
                    face.create_from_file("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf") == BL_SUCCESS ||
                    face.create_from_file("/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf") == BL_SUCCESS) {
                    BLFont font;
                    font.create_from_face(face, 24.0f);
                    ctx.set_fill_style(BLRgba32(0xFFFFFFFF));
                    ctx.fill_utf8_text(BLPoint(cx - (w * 0.4), cy + (h * 0.4)), font, "Blend2D Hybrid Vector Text");
                }
                ctx.end();

                // --- Composite back to Destination ---
                BLImageData img_data;
                img.get_data(&img_data);

                for (int y = 0; y < height; y++) {
                    uint32_t* src_line = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(img_data.pixel_data) +
                                                                     (y * img_data.stride));

                    // Flip Y for Destination: OpenFX y=0 is Bottom, Blend2D y=0 is Top.
                    int dst_y = (height - 1) - y;

                    if (dstBitDepth == OFX::eBitDepthUByte) {
                        uint8_t* dst_line =
                            reinterpret_cast<uint8_t*>(dst->getPixelAddress(bounds.x1, bounds.y1 + dst_y));
                        for (int x = 0; x < width; x++) {
                            uint32_t pixel = src_line[x];
                            uint8_t a = static_cast<uint8_t>((pixel >> 24) & 0xFF);
                            if (a > 0) {
                                // Simple Alpha Blend (8-bit)
                                for (int c = 0; c < 4; c++) {
                                    uint8_t s = static_cast<uint8_t>((pixel >> (c * 8)) & 0xFF);
                                    uint8_t d = dst_line[x * 4 + c];
                                    dst_line[x * 4 + c] = static_cast<uint8_t>((s * 255 + d * (255 - a)) / 255);
                                }
                            }
                        }
                    } else if (dstBitDepth == OFX::eBitDepthFloat) {
                        float* dst_line = reinterpret_cast<float*>(dst->getPixelAddress(bounds.x1, bounds.y1 + dst_y));
                        for (int x = 0; x < width; x++) {
                            uint32_t pixel = src_line[x];
                            float a = static_cast<float>((pixel >> 24) & 0xFF) / 255.0f;
                            if (a > 0.0f) {
                                // Simple Alpha Blend (Float)
                                for (int c = 0; c < 4; c++) {
                                    float s = static_cast<float>((pixel >> (c * 8)) & 0xFF) / 255.0f;
                                    float d = dst_line[x * 4 + c];
                                    dst_line[x * 4 + c] = s + d * (1.0f - a);
                                }
                            }
                        }
                    }
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