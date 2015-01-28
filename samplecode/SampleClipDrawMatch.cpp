/*
 * Copyright 2015 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SampleCode.h"
#include "SkCanvas.h"
#include "SkInterpolator.h"
#include "SkTime.h"

// This slide tests out the match up between BW clipping and rendering. It can
// draw a large rect through some clip geometry and draw the same geometry
// normally. Which one is drawn first can be toggled. The pair of objects is translated
// fractionally (via an animator) to expose snapping bugs. The key bindings are:
//      1-9: the different geometries
//      t:   toggle which is drawn first the clip or the normal geometry

// The possible geometric combinations to test
enum Geometry {
    kRect_Geometry,
    kRRect_Geometry,
    kCircle_Geometry,
    kConvexPath_Geometry,
    kConcavePath_Geometry,
    kRectAndRect_Geometry,
    kRectAndRRect_Geometry,
    kRectAndConvex_Geometry,
    kRectAndConcave_Geometry
};

// The basic rect used is [kMin,kMin]..[kMax,kMax]
static const float kMin = 100.5f;
static const float kMid = 200.0f;
static const float kMax = 299.5f;

SkRect create_rect(const SkPoint& offset) {
    SkRect r = SkRect::MakeLTRB(kMin, kMin, kMax, kMax);
    r.offset(offset);
    return r;
}

SkRRect create_rrect(const SkPoint& offset) {
    SkRRect rrect;
    rrect.setRectXY(create_rect(offset), 10, 10);
    return rrect;
}

SkRRect create_circle(const SkPoint& offset) {
    SkRRect circle;
    circle.setOval(create_rect(offset));
    return circle;
}

SkPath create_convex_path(const SkPoint& offset) {
    SkPath convexPath;
    convexPath.moveTo(kMin, kMin);
    convexPath.lineTo(kMax, kMax);
    convexPath.lineTo(kMin, kMax);
    convexPath.close();
    convexPath.offset(offset.fX, offset.fY);
    return convexPath;
}

SkPath create_concave_path(const SkPoint& offset) {
    SkPath concavePath;
    concavePath.moveTo(kMin, kMin);
    concavePath.lineTo(kMid, 105.0f);
    concavePath.lineTo(kMax, kMin);
    concavePath.lineTo(295.0f, kMid);
    concavePath.lineTo(kMax, kMax);
    concavePath.lineTo(kMid, 295.0f);
    concavePath.lineTo(kMin, kMax);
    concavePath.lineTo(105.0f, kMid);
    concavePath.close();

    concavePath.offset(offset.fX, offset.fY);
    return concavePath;
}

static void draw_clipped_geom(SkCanvas* canvas, const SkPoint& offset, int geom, bool useAA) {

    int count = canvas->save();

    switch (geom) {
    case kRect_Geometry:
        canvas->clipRect(create_rect(offset), SkRegion::kReplace_Op, useAA);
        break;
    case kRRect_Geometry:
        canvas->clipRRect(create_rrect(offset), SkRegion::kReplace_Op, useAA);
        break;
    case kCircle_Geometry:
        canvas->clipRRect(create_circle(offset), SkRegion::kReplace_Op, useAA);
        break;
    case kConvexPath_Geometry:
        canvas->clipPath(create_convex_path(offset), SkRegion::kReplace_Op, useAA);
        break;
    case kConcavePath_Geometry:
        canvas->clipPath(create_concave_path(offset), SkRegion::kReplace_Op, useAA);
        break;
    case kRectAndRect_Geometry: {
        SkRect r = create_rect(offset);
        r.offset(-100.0f, -100.0f);
        canvas->clipRect(r, SkRegion::kReplace_Op, true); // AA here forces shader clips
        canvas->clipRect(create_rect(offset), SkRegion::kIntersect_Op, useAA);
        } break;
    case kRectAndRRect_Geometry: {
        SkRect r = create_rect(offset);
        r.offset(-100.0f, -100.0f);
        canvas->clipRect(r, SkRegion::kReplace_Op, true); // AA here forces shader clips
        canvas->clipRRect(create_rrect(offset), SkRegion::kIntersect_Op, useAA);
        } break;
    case kRectAndConvex_Geometry: {
        SkRect r = create_rect(offset);
        r.offset(-100.0f, -100.0f);
        canvas->clipRect(r, SkRegion::kReplace_Op, true); // AA here forces shader clips
        canvas->clipPath(create_convex_path(offset), SkRegion::kIntersect_Op, useAA);
        } break;
    case kRectAndConcave_Geometry: {
        SkRect r = create_rect(offset);
        r.offset(-100.0f, -100.0f);
        canvas->clipRect(r, SkRegion::kReplace_Op, true); // AA here forces shader clips
        canvas->clipPath(create_concave_path(offset), SkRegion::kIntersect_Op, useAA);
        } break;
    } 

    SkISize size = canvas->getDeviceSize();
    SkRect bigR = SkRect::MakeWH(SkIntToScalar(size.width()), SkIntToScalar(size.height()));

    SkPaint p;
    p.setColor(SK_ColorRED);

    canvas->drawRect(bigR, p);
    canvas->restoreToCount(count);
}

static void draw_normal_geom(SkCanvas* canvas, const SkPoint& offset, int geom, bool useAA) {
    SkPaint p;
    p.setAntiAlias(useAA);
    p.setColor(SK_ColorBLACK);

    switch (geom) {
    case kRect_Geometry:                // fall thru
    case kRectAndRect_Geometry:
        canvas->drawRect(create_rect(offset), p);
        break;
    case kRRect_Geometry:               // fall thru
    case kRectAndRRect_Geometry:
        canvas->drawRRect(create_rrect(offset), p);
        break;
    case kCircle_Geometry:
        canvas->drawRRect(create_circle(offset), p);
        break;
    case kConvexPath_Geometry:          // fall thru
    case kRectAndConvex_Geometry:
        canvas->drawPath(create_convex_path(offset), p);
        break;
    case kConcavePath_Geometry:         // fall thru
    case kRectAndConcave_Geometry:
        canvas->drawPath(create_concave_path(offset), p);
        break;
    } 
}

class ClipDrawMatchView : public SampleView {
public:
    ClipDrawMatchView() : fTrans(2, 5), fGeom(kRect_Geometry), fClipFirst(true) {
        SkScalar values[2];

        fTrans.setRepeatCount(999);
        values[0] = values[1] = 0;
        fTrans.setKeyFrame(0, SkTime::GetMSecs() + 1000, values);
        values[1] = 1;
        fTrans.setKeyFrame(1, SkTime::GetMSecs() + 2000, values);
        values[0] = values[1] = 1;
        fTrans.setKeyFrame(2, SkTime::GetMSecs() + 3000, values);
        values[1] = 0;
        fTrans.setKeyFrame(3, SkTime::GetMSecs() + 4000, values);
        values[0] = 0;
        fTrans.setKeyFrame(4, SkTime::GetMSecs() + 5000, values);
    }

protected:
    bool onQuery(SkEvent* evt) SK_OVERRIDE {
        if (SampleCode::TitleQ(*evt)) {
            SampleCode::TitleR(evt, "ClipDrawMatch");
            return true;
        }
        SkUnichar uni;
        if (SampleCode::CharQ(*evt, &uni)) {
            switch (uni) {
                case '1': fGeom = kRect_Geometry; this->inval(NULL); return true;
                case '2': fGeom = kRRect_Geometry; this->inval(NULL); return true;
                case '3': fGeom = kCircle_Geometry; this->inval(NULL); return true;
                case '4': fGeom = kConvexPath_Geometry; this->inval(NULL); return true;
                case '5': fGeom = kConcavePath_Geometry; this->inval(NULL); return true;
                case '6': fGeom = kRectAndRect_Geometry; this->inval(NULL); return true;
                case '7': fGeom = kRectAndRRect_Geometry; this->inval(NULL); return true;
                case '8': fGeom = kRectAndConvex_Geometry; this->inval(NULL); return true;
                case '9': fGeom = kRectAndConcave_Geometry; this->inval(NULL); return true;
                case 't': fClipFirst = !fClipFirst; this->inval(NULL); return true;
                default: break;
            }
        }
        return this->INHERITED::onQuery(evt);
    }

    // Draw a big red rect through some clip geometry and also draw that same
    // geometry in black. The order in which they are drawn can be swapped.
    // This tests whether the clip and normally drawn geometry match up.
    void drawGeometry(SkCanvas* canvas, const SkPoint& offset, bool useAA) {
        if (fClipFirst) {
            draw_clipped_geom(canvas, offset, fGeom, useAA);
        }

        draw_normal_geom(canvas, offset, fGeom, useAA);

        if (!fClipFirst) {
            draw_clipped_geom(canvas, offset, fGeom, useAA);
        }
    }

    void onDrawContent(SkCanvas* canvas) SK_OVERRIDE {
        SkScalar trans[2];
        fTrans.timeToValues(SkTime::GetMSecs(), trans);

        SkPoint offset;
        offset.set(trans[0], trans[1]);

        int saveCount = canvas->save();
        this->drawGeometry(canvas, offset, false);
        canvas->restoreToCount(saveCount);

        this->inval(NULL);
    }

private:
    SkInterpolator  fTrans;
    Geometry        fGeom;
    bool            fClipFirst;

    typedef SampleView INHERITED;
};

//////////////////////////////////////////////////////////////////////////////

static SkView* MyFactory() { return new ClipDrawMatchView; }
static SkViewRegister reg(MyFactory);