
//
// CanvasRenderingContext2d.cc
//
// Copyright (c) 2010 LearnBoost <tj@learnboost.com>
//

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "Canvas.h"
#include "Point.h"
#include "Image.h"
#include "ImageData.h"
#include "CanvasRenderingContext2d.h"
#include "CanvasGradient.h"

Persistent<FunctionTemplate> Context2d::constructor;

/*
 * Rectangle arg assertions.
 */

#define RECT_ARGS \
  if (!args[0]->IsNumber() \
    ||!args[1]->IsNumber() \
    ||!args[2]->IsNumber() \
    ||!args[3]->IsNumber()) return Undefined(); \
  double x = args[0]->Int32Value(); \
  double y = args[1]->Int32Value(); \
  double width = args[2]->Int32Value(); \
  double height = args[3]->Int32Value();

/*
 * Text baselines.
 */

enum {
    TEXT_BASELINE_ALPHABETIC
  , TEXT_BASELINE_TOP
  , TEXT_BASELINE_BOTTOM
  , TEXT_BASELINE_MIDDLE
  , TEXT_BASELINE_IDEOGRAPHIC
  , TEXT_BASELINE_HANGING
};

/*
 * Initialize Context2d.
 */

void
Context2d::Initialize(Handle<Object> target) {
  HandleScope scope;

  // Constructor
  constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(Context2d::New));
  constructor->InstanceTemplate()->SetInternalFieldCount(1);
  constructor->SetClassName(String::NewSymbol("CanvasRenderingContext2d"));

  // Prototype
  Local<ObjectTemplate> proto = constructor->PrototypeTemplate();
  NODE_SET_PROTOTYPE_METHOD(constructor, "drawImage", DrawImage);
  NODE_SET_PROTOTYPE_METHOD(constructor, "putImageData", PutImageData);
  NODE_SET_PROTOTYPE_METHOD(constructor, "save", Save);
  NODE_SET_PROTOTYPE_METHOD(constructor, "restore", Restore);
  NODE_SET_PROTOTYPE_METHOD(constructor, "rotate", Rotate);
  NODE_SET_PROTOTYPE_METHOD(constructor, "translate", Translate);
  NODE_SET_PROTOTYPE_METHOD(constructor, "transform", Transform);
  NODE_SET_PROTOTYPE_METHOD(constructor, "resetTransform", ResetTransform);
  NODE_SET_PROTOTYPE_METHOD(constructor, "isPointInPath", IsPointInPath);
  NODE_SET_PROTOTYPE_METHOD(constructor, "scale", Scale);
  NODE_SET_PROTOTYPE_METHOD(constructor, "clip", Clip);
  NODE_SET_PROTOTYPE_METHOD(constructor, "fill", Fill);
  NODE_SET_PROTOTYPE_METHOD(constructor, "stroke", Stroke);
  NODE_SET_PROTOTYPE_METHOD(constructor, "fillText", FillText);
  NODE_SET_PROTOTYPE_METHOD(constructor, "strokeText", StrokeText);
  NODE_SET_PROTOTYPE_METHOD(constructor, "fillRect", FillRect);
  NODE_SET_PROTOTYPE_METHOD(constructor, "strokeRect", StrokeRect);
  NODE_SET_PROTOTYPE_METHOD(constructor, "clearRect", ClearRect);
  NODE_SET_PROTOTYPE_METHOD(constructor, "rect", Rect);
  NODE_SET_PROTOTYPE_METHOD(constructor, "setTextBaseline", SetTextBaseline);
  NODE_SET_PROTOTYPE_METHOD(constructor, "setTextAlignment", SetTextAlignment);
  NODE_SET_PROTOTYPE_METHOD(constructor, "measureText", MeasureText);
  NODE_SET_PROTOTYPE_METHOD(constructor, "moveTo", MoveTo);
  NODE_SET_PROTOTYPE_METHOD(constructor, "lineTo", LineTo);
  NODE_SET_PROTOTYPE_METHOD(constructor, "bezierCurveTo", BezierCurveTo);
  NODE_SET_PROTOTYPE_METHOD(constructor, "quadraticCurveTo", QuadraticCurveTo);
  NODE_SET_PROTOTYPE_METHOD(constructor, "beginPath", BeginPath);
  NODE_SET_PROTOTYPE_METHOD(constructor, "closePath", ClosePath);
  NODE_SET_PROTOTYPE_METHOD(constructor, "arc", Arc);
  NODE_SET_PROTOTYPE_METHOD(constructor, "arcTo", ArcTo);
  NODE_SET_PROTOTYPE_METHOD(constructor, "setFont", SetFont);
  NODE_SET_PROTOTYPE_METHOD(constructor, "setFillColor", SetFillColor);
  NODE_SET_PROTOTYPE_METHOD(constructor, "setStrokeColor", SetStrokeColor);
  NODE_SET_PROTOTYPE_METHOD(constructor, "setFillPattern", SetFillPattern);
  NODE_SET_PROTOTYPE_METHOD(constructor, "setStrokePattern", SetStrokePattern);
  proto->SetAccessor(String::NewSymbol("patternQuality"), GetPatternQuality, SetPatternQuality);
  proto->SetAccessor(String::NewSymbol("globalCompositeOperation"), GetGlobalCompositeOperation, SetGlobalCompositeOperation);
  proto->SetAccessor(String::NewSymbol("globalAlpha"), GetGlobalAlpha, SetGlobalAlpha);
  proto->SetAccessor(String::NewSymbol("shadowColor"), GetShadowColor, SetShadowColor);
  proto->SetAccessor(String::NewSymbol("fillColor"), GetFillColor);
  proto->SetAccessor(String::NewSymbol("strokeColor"), GetStrokeColor);
  proto->SetAccessor(String::NewSymbol("miterLimit"), GetMiterLimit, SetMiterLimit);
  proto->SetAccessor(String::NewSymbol("lineWidth"), GetLineWidth, SetLineWidth);
  proto->SetAccessor(String::NewSymbol("lineCap"), GetLineCap, SetLineCap);
  proto->SetAccessor(String::NewSymbol("lineJoin"), GetLineJoin, SetLineJoin);
  proto->SetAccessor(String::NewSymbol("shadowOffsetX"), GetShadowOffsetX, SetShadowOffsetX);
  proto->SetAccessor(String::NewSymbol("shadowOffsetY"), GetShadowOffsetY, SetShadowOffsetY);
  proto->SetAccessor(String::NewSymbol("shadowBlur"), GetShadowBlur, SetShadowBlur);
  proto->SetAccessor(String::NewSymbol("antialias"), GetAntiAlias, SetAntiAlias);
  target->Set(String::NewSymbol("CanvasRenderingContext2d"), constructor->GetFunction());
}

/*
 * Create a cairo context.
 */

Context2d::Context2d(Canvas *canvas) {
  _canvas = canvas;
  _context = cairo_create(canvas->surface());
  cairo_set_line_width(_context, 1);
  state = states[stateno = 0] = (canvas_state_t *) malloc(sizeof(canvas_state_t));
  state->shadowBlur = 0;
  state->shadowOffsetX = state->shadowOffsetY = 0;
  state->globalAlpha = 1;
  state->textAlignment = -1;
  state->fillPattern = state->strokePattern = NULL;
  state->textBaseline = NULL;
  rgba_t transparent = { 0,0,0,1 };
  rgba_t transparent_black = { 0,0,0,0 };
  state->fill = transparent;
  state->stroke = transparent;
  state->shadow = transparent_black;
  state->patternQuality = CAIRO_FILTER_GOOD;
}

/*
 * Destroy cairo context.
 */

Context2d::~Context2d() {
  cairo_destroy(_context);
}

/*
 * Save cairo / canvas state.
 */

void
Context2d::save() {
  cairo_save(_context);
  saveState();
}

/*
 * Restore cairo / canvas state.
 */

void
Context2d::restore() {
  cairo_restore(_context);
  restoreState();
}

/*
 * Save the current state.
 */

void
Context2d::saveState() {
  if (stateno == CANVAS_MAX_STATES) return;
  states[++stateno] = (canvas_state_t *) malloc(sizeof(canvas_state_t));
  memcpy(states[stateno], state, sizeof(canvas_state_t));
  state = states[stateno];
}

/*
 * Restore state.
 */

void
Context2d::restoreState() {
  if (0 == stateno) return;
  state = states[--stateno];
}

/*
 * Save flat path.
 */

void
Context2d::savePath() {
  _path = cairo_copy_path_flat(_context);
  cairo_new_path(_context);
}

/*
 * Restore flat path.
 */

void
Context2d::restorePath() {
  cairo_append_path(_context, _path);
}

/*
 * Fill and apply shadow.
 */

void
Context2d::fill(bool preserve) {
  if (state->fillPattern) {
    cairo_pattern_set_filter(state->fillPattern, state->patternQuality);
    cairo_set_source(_context, state->fillPattern);
  } else {
    setSourceRGBA(state->fill);
  }

  if (preserve) {
    hasShadow()
      ? shadow(cairo_fill_preserve)
      : cairo_fill_preserve(_context);
  } else {
    hasShadow()
      ? shadow(cairo_fill)
      : cairo_fill(_context);
  }
}

/*
 * Stroke and apply shadow.
 */

void
Context2d::stroke(bool preserve) {
  if (state->strokePattern) {
    cairo_pattern_set_filter(state->strokePattern, state->patternQuality);
    cairo_set_source(_context, state->fillPattern);
  } else {
    setSourceRGBA(state->stroke);
  }

  if (preserve) {
    hasShadow()
      ? shadow(cairo_stroke_preserve)
      : cairo_stroke_preserve(_context);
  } else {
    hasShadow()
      ? shadow(cairo_stroke)
      : cairo_stroke(_context);
  }
}

/*
 * Apply shadow with the given draw fn.
 */

void
Context2d::shadow(void (fn)(cairo_t *cr)) {
  cairo_path_t *path = cairo_copy_path_flat(_context);
  cairo_save(_context);

  // Offset
  cairo_translate(
      _context
    , state->shadowOffsetX
    , state->shadowOffsetY);

  // Apply shadow
  cairo_push_group(_context);
  cairo_new_path(_context);
  cairo_append_path(_context, path);
  setSourceRGBA(state->shadow);
  fn(_context);

  // No need to invoke blur if shadowBlur is 0
  if (state->shadowBlur) {
    blur(cairo_get_group_target(_context), state->shadowBlur);
  }

  // Paint the shadow
  cairo_pop_group_to_source(_context);
  cairo_paint(_context);

  // Restore state
  cairo_restore(_context);
  cairo_new_path(_context);
  cairo_append_path(_context, path);
  fn(_context);

  cairo_path_destroy(path);
}

/*
 * Set source RGBA.
 */

void
Context2d::setSourceRGBA(rgba_t color) {
  cairo_set_source_rgba(
      _context
    , color.r
    , color.g
    , color.b
    , color.a * state->globalAlpha);
}

/*
 * Check if the context has a drawable shadow.
 */

bool
Context2d::hasShadow() {
  return state->shadow.a
    && (state->shadowBlur || state->shadowOffsetX || state->shadowOffsetX);
}

/*
 * Blur the given surface with the given radius.
 */

void
Context2d::blur(cairo_surface_t *surface, int radius) {
  // Steve Hanov, 2009
  // Released into the public domain.
  --radius;
  // get width, height
  int width = cairo_image_surface_get_width( surface );
  int height = cairo_image_surface_get_height( surface );
  unsigned* precalc = 
      (unsigned*)malloc(width*height*sizeof(unsigned));
  unsigned char* src = cairo_image_surface_get_data( surface );
  double mul=1.f/((radius*2)*(radius*2));
  int channel;
  
  // The number of times to perform the averaging. According to wikipedia,
  // three iterations is good enough to pass for a gaussian.
  const int MAX_ITERATIONS = 3; 
  int iteration;

  for ( iteration = 0; iteration < MAX_ITERATIONS; iteration++ ) {
      for( channel = 0; channel < 4; channel++ ) {
          int x,y;

          // precomputation step.
          unsigned char* pix = src;
          unsigned* pre = precalc;

          pix += channel;
          for (y=0;y<height;y++) {
              for (x=0;x<width;x++) {
                  int tot=pix[0];
                  if (x>0) tot+=pre[-1];
                  if (y>0) tot+=pre[-width];
                  if (x>0 && y>0) tot-=pre[-width-1];
                  *pre++=tot;
                  pix += 4;
              }
          }

          // blur step.
          pix = src + (int)radius * width * 4 + (int)radius * 4 + channel;
          for (y=radius;y<height-radius;y++) {
              for (x=radius;x<width-radius;x++) {
                  int l = x < radius ? 0 : x - radius;
                  int t = y < radius ? 0 : y - radius;
                  int r = x + radius >= width ? width - 1 : x + radius;
                  int b = y + radius >= height ? height - 1 : y + radius;
                  int tot = precalc[r+b*width] + precalc[l+t*width] - 
                      precalc[l+b*width] - precalc[r+t*width];
                  *pix=(unsigned char)(tot*mul);
                  pix += 4;
              }
              pix += (int)radius * 2 * 4;
          }
      }
  }
  free( precalc );
}

/*
 * Initialize a new Context2d with the given canvas.
 */

Handle<Value>
Context2d::New(const Arguments &args) {
  HandleScope scope;
  Local<Object> obj = args[0]->ToObject();
  if (!Canvas::constructor->HasInstance(obj))
    return ThrowException(Exception::TypeError(String::New("Canvas expected")));
  Canvas *canvas = ObjectWrap::Unwrap<Canvas>(obj);
  Context2d *context = new Context2d(canvas);
  context->Wrap(args.This());
  return args.This();
}

/*
 * Put image data.
 *
 *  - imageData, dx, dy
 *  - imageData, dx, dy, sx, sy, sw, sh
 *
 */

Handle<Value>
Context2d::PutImageData(const Arguments &args) {
  HandleScope scope;

  Local<Object> obj = args[0]->ToObject();
  if (!ImageData::constructor->HasInstance(obj))
    return ThrowException(Exception::TypeError(String::New("ImageData expected")));

  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  ImageData *imageData = ObjectWrap::Unwrap<ImageData>(obj);
  PixelArray *arr = imageData->pixelArray();
  
  uint8_t *src = arr->data();
  uint8_t *dst = context->canvas()->data();

  int srcStride = arr->stride()
    , dstStride = context->canvas()->stride();

  int sx = 0
    , sy = 0
    , sw = 0
    , sh = 0
    , dx = args[1]->Int32Value()
    , dy = args[2]->Int32Value()
    , rows
    , cols;

  switch (args.Length()) {
    // imageData, dx, dy
    case 3:
      cols = arr->width();
      rows = arr->height();
      break;
    // imageData, dx, dy, sx, sy, sw, sh
    case 7:
      sx = args[3]->Int32Value();
      sy = args[4]->Int32Value();
      sw = args[5]->Int32Value();
      sh = args[6]->Int32Value();
      if (sx < 0) sw += sx, sx = 0;
      if (sy < 0) sh += sy, sy = 0;
      if (sx + sw > arr->width()) sw = arr->width() - sx;
      if (sy + sh > arr->height()) sh = arr->height() - sy;
      if (sw <= 0 || sh <= 0) return Undefined();
      cols = sw;
      rows = sh;
      dx += sx; 
      dy += sy; 
      break;
    default:
      return ThrowException(Exception::Error(String::New("invalid arguments")));
  }

  uint8_t *srcRows = src + sy * srcStride + sx * 4;
  for (int y = 0; y < rows; ++y) {
    uint32_t *row = (uint32_t *)(dst + dstStride * (y + dy));
    for (int x = 0; x < cols; ++x) {
      int bx = x * 4;
      uint32_t *pixel = row + x + dx;

      // RGBA
      uint8_t a = srcRows[bx + 3];
      uint8_t r = srcRows[bx + 0];
      uint8_t g = srcRows[bx + 1];
      uint8_t b = srcRows[bx + 2];
      float alpha = (float) a / 255;

      // ARGB
      *pixel = a << 24
        | (int)((float) r * alpha) << 16
        | (int)((float) g * alpha) << 8
        | (int)((float) b * alpha);
    }
    srcRows += srcStride;
  }

  cairo_surface_mark_dirty_rectangle(
      context->canvas()->surface()
    , dx
    , dy
    , cols
    , rows);

  return Undefined();
}

/*
 * Draw image src image to the destination (context).
 *
 *  - dx, dy
 *  - dx, dy, dw, dh
 *  - sx, sy, sw, sh, dx, dy, dw, dh
 *
 */

Handle<Value>
Context2d::DrawImage(const Arguments &args) {
  HandleScope scope;

  if (args.Length() < 3)
    return ThrowException(Exception::TypeError(String::New("invalid arguments")));

#if CAIRO_VERSION_MINOR < 10
  return ThrowException(Exception::Error(String::New("drawImage() needs cairo >= 1.10.0")));
#else

  Local<Object> obj = args[0]->ToObject();
  if (!Image::constructor->HasInstance(obj))
    return ThrowException(Exception::TypeError(String::New("Image expected")));

  Image *img = ObjectWrap::Unwrap<Image>(obj);
  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  cairo_t *ctx = context->context();

  int sx = 0
    , sy = 0
    , sw = img->width
    , sh = img->height
    , dx, dy, dw, dh;

  // Arguments
  switch (args.Length()) {
    // img, sx, sy, sw, sh, dx, dy, dw, dh
    case 9:
      sx = args[1]->Int32Value();
      sy = args[2]->Int32Value();
      sw = args[3]->Int32Value();
      sh = args[4]->Int32Value();
      dx = args[5]->Int32Value();
      dy = args[6]->Int32Value();
      dw = args[7]->Int32Value();
      dh = args[8]->Int32Value();
      break;
    // img, dx, dy, dw, dh
    case 5:
      dx = args[1]->Int32Value();
      dy = args[2]->Int32Value();
      dw = args[3]->Int32Value();
      dh = args[4]->Int32Value();
      break;
    // img, dx, dy
    case 3:
      dx = args[1]->Int32Value();
      dy = args[2]->Int32Value();
      dw = img->width;
      dh = img->height;
      break;
    default:
      return ThrowException(Exception::TypeError(String::New("invalid arguments")));
  }

  // Start draw
  cairo_save(ctx);

  // Source surface
  // TODO: only works with cairo >= 1.10.0
  cairo_surface_t *src = cairo_surface_create_for_rectangle(
      img->surface()
    , sx
    , sy
    , sw
    , sh);

  // Scale src
  if (dw != sw || dh != sh) {
    float fx = (float) dw / sw;
    float fy = (float) dh / sh;
    cairo_scale(ctx, fx, fy);
    dx /= fx;
    dy /= fy;
  }

  // Paint
  cairo_set_source_surface(ctx, src, dx, dy);
  cairo_pattern_set_filter(cairo_get_source(ctx), context->state->patternQuality);
  cairo_paint_with_alpha(ctx, context->state->globalAlpha);

  cairo_restore(ctx);
  cairo_surface_destroy(src);

#endif

  return Undefined();
}

/*
 * Get global alpha.
 */

Handle<Value>
Context2d::GetGlobalAlpha(Local<String> prop, const AccessorInfo &info) {
  Context2d *context = ObjectWrap::Unwrap<Context2d>(info.This());
  return Number::New(context->state->globalAlpha);
}

/*
 * Set global alpha.
 */

void
Context2d::SetGlobalAlpha(Local<String> prop, Local<Value> val, const AccessorInfo &info) {
  double n = val->NumberValue();
  if (n >= 0 && n <= 1) {
    Context2d *context = ObjectWrap::Unwrap<Context2d>(info.This());
    context->state->globalAlpha = n;
  }
}

/*
 * Get global composite operation.
 */

Handle<Value>
Context2d::GetGlobalCompositeOperation(Local<String> prop, const AccessorInfo &info) {
  Context2d *context = ObjectWrap::Unwrap<Context2d>(info.This());
  cairo_t *ctx = context->context();
  switch (cairo_get_operator(ctx)) {
    case CAIRO_OPERATOR_ATOP:
      return String::NewSymbol("source-atop");
    case CAIRO_OPERATOR_IN:
      return String::NewSymbol("source-in");
    case CAIRO_OPERATOR_OUT:
      return String::NewSymbol("source-out");
    case CAIRO_OPERATOR_XOR:
      return String::NewSymbol("xor");
    case CAIRO_OPERATOR_DEST_ATOP:
      return String::NewSymbol("destination-atop");
    case CAIRO_OPERATOR_DEST_IN:
      return String::NewSymbol("destination-in");
    case CAIRO_OPERATOR_DEST_OUT:
      return String::NewSymbol("destination-out");
    case CAIRO_OPERATOR_DEST_OVER:
      return String::NewSymbol("destination-over");
    case CAIRO_OPERATOR_ADD:
      return String::NewSymbol("lighter");
    // Non-standard
    // supported by resent versions of cairo
#if CAIRO_VERSION_MINOR >= 10
    case CAIRO_OPERATOR_LIGHTEN:
      return String::NewSymbol("lighter");
    case CAIRO_OPERATOR_DARKEN:
      return String::NewSymbol("darkler");
    case CAIRO_OPERATOR_MULTIPLY:
      return String::NewSymbol("multiply");
    case CAIRO_OPERATOR_SCREEN:
      return String::NewSymbol("screen");
    case CAIRO_OPERATOR_OVERLAY:
      return String::NewSymbol("overlay");
    case CAIRO_OPERATOR_HARD_LIGHT:
      return String::NewSymbol("hard-light");
    case CAIRO_OPERATOR_SOFT_LIGHT:
      return String::NewSymbol("soft-light");
    case CAIRO_OPERATOR_HSL_HUE:
      return String::NewSymbol("hsl-hue");
    case CAIRO_OPERATOR_HSL_SATURATION:
      return String::NewSymbol("hsl-saturation");
    case CAIRO_OPERATOR_HSL_COLOR:
      return String::NewSymbol("hsl-color");
    case CAIRO_OPERATOR_HSL_LUMINOSITY:
      return String::NewSymbol("hsl-luminosity");
#endif
    default:
      return String::NewSymbol("source-over");
  }
}

/*
 * Set pattern quality.
 */

void
Context2d::SetPatternQuality(Local<String> prop, Local<Value> val, const AccessorInfo &info) {
  Context2d *context = ObjectWrap::Unwrap<Context2d>(info.This());
  String::AsciiValue quality(val->ToString());
  if (0 == strcmp("fast", *quality)) {
    context->state->patternQuality = CAIRO_FILTER_FAST;
  } else if (0 == strcmp("good", *quality)) {
    context->state->patternQuality = CAIRO_FILTER_GOOD;
  } else if (0 == strcmp("best", *quality)) {
    context->state->patternQuality = CAIRO_FILTER_BEST;
  }
}

/*
 * Get pattern quality.
 */

Handle<Value>
Context2d::GetPatternQuality(Local<String> prop, const AccessorInfo &info) {
  Context2d *context = ObjectWrap::Unwrap<Context2d>(info.This());
  switch (context->state->patternQuality) {
    case CAIRO_FILTER_FAST:
      return String::New("fast");
    case CAIRO_FILTER_BEST:
      return String::New("best");
    default:  
      return String::New("good");
  }
}

/*
 * Set global composite operation.
 */

void
Context2d::SetGlobalCompositeOperation(Local<String> prop, Local<Value> val, const AccessorInfo &info) {
  Context2d *context = ObjectWrap::Unwrap<Context2d>(info.This());
  cairo_t *ctx = context->context();
  String::AsciiValue type(val->ToString());
  if (0 == strcmp("xor", *type)) {
    cairo_set_operator(ctx, CAIRO_OPERATOR_XOR);
  } else if (0 == strcmp("source-atop", *type)) {
    cairo_set_operator(ctx, CAIRO_OPERATOR_ATOP);
  } else if (0 == strcmp("source-in", *type)) {
    cairo_set_operator(ctx, CAIRO_OPERATOR_IN);
  } else if (0 == strcmp("source-out", *type)) {
    cairo_set_operator(ctx, CAIRO_OPERATOR_OUT);
  } else if (0 == strcmp("destination-atop", *type)) {
    cairo_set_operator(ctx, CAIRO_OPERATOR_DEST_ATOP);
  } else if (0 == strcmp("destination-in", *type)) {
    cairo_set_operator(ctx, CAIRO_OPERATOR_DEST_IN);
  } else if (0 == strcmp("destination-out", *type)) {
    cairo_set_operator(ctx, CAIRO_OPERATOR_DEST_OUT);
  } else if (0 == strcmp("destination-over", *type)) {
    cairo_set_operator(ctx, CAIRO_OPERATOR_DEST_OVER);
  // Non-standard
  // supported by resent versions of cairo
#if CAIRO_VERSION_MINOR >= 10
  } else if (0 == strcmp("lighter", *type)) {
    cairo_set_operator(ctx, CAIRO_OPERATOR_LIGHTEN);
  } else if (0 == strcmp("darker", *type)) {
    cairo_set_operator(ctx, CAIRO_OPERATOR_DARKEN);
  } else if (0 == strcmp("multiply", *type)) {
    cairo_set_operator(ctx, CAIRO_OPERATOR_MULTIPLY);
  } else if (0 == strcmp("screen", *type)) {
    cairo_set_operator(ctx, CAIRO_OPERATOR_SCREEN);
  } else if (0 == strcmp("overlay", *type)) {
    cairo_set_operator(ctx, CAIRO_OPERATOR_OVERLAY);
  } else if (0 == strcmp("hard-light", *type)) {
    cairo_set_operator(ctx, CAIRO_OPERATOR_HARD_LIGHT);
  } else if (0 == strcmp("soft-light", *type)) {
    cairo_set_operator(ctx, CAIRO_OPERATOR_SOFT_LIGHT);
  } else if (0 == strcmp("hsl-hue", *type)) {
    cairo_set_operator(ctx, CAIRO_OPERATOR_HSL_HUE);
  } else if (0 == strcmp("hsl-saturation", *type)) {
    cairo_set_operator(ctx, CAIRO_OPERATOR_HSL_SATURATION);
  } else if (0 == strcmp("hsl-color", *type)) {
    cairo_set_operator(ctx, CAIRO_OPERATOR_HSL_COLOR);
  } else if (0 == strcmp("hsl-luminosity", *type)) {
    cairo_set_operator(ctx, CAIRO_OPERATOR_HSL_LUMINOSITY);
#endif
  } else if (0 == strcmp("lighter", *type)) {
    cairo_set_operator(ctx, CAIRO_OPERATOR_ADD);
  } else {
    cairo_set_operator(ctx, CAIRO_OPERATOR_OVER);
  }
}

/*
 * Get shadow offset x.
 */

Handle<Value>
Context2d::GetShadowOffsetX(Local<String> prop, const AccessorInfo &info) {
  Context2d *context = ObjectWrap::Unwrap<Context2d>(info.This());
  return Number::New(context->state->shadowOffsetX);
}

/*
 * Set shadow offset x.
 */

void
Context2d::SetShadowOffsetX(Local<String> prop, Local<Value> val, const AccessorInfo &info) {
  Context2d *context = ObjectWrap::Unwrap<Context2d>(info.This());
  context->state->shadowOffsetX = val->NumberValue();
}

/*
 * Get shadow offset y.
 */

Handle<Value>
Context2d::GetShadowOffsetY(Local<String> prop, const AccessorInfo &info) {
  Context2d *context = ObjectWrap::Unwrap<Context2d>(info.This());
  return Number::New(context->state->shadowOffsetY);
}

/*
 * Set shadow offset y.
 */

void
Context2d::SetShadowOffsetY(Local<String> prop, Local<Value> val, const AccessorInfo &info) {
  Context2d *context = ObjectWrap::Unwrap<Context2d>(info.This());
  context->state->shadowOffsetY = val->NumberValue();
}

/*
 * Get shadow blur.
 */

Handle<Value>
Context2d::GetShadowBlur(Local<String> prop, const AccessorInfo &info) {
  Context2d *context = ObjectWrap::Unwrap<Context2d>(info.This());
  return Number::New(context->state->shadowBlur);
}

/*
 * Set shadow blur.
 */

void
Context2d::SetShadowBlur(Local<String> prop, Local<Value> val, const AccessorInfo &info) {
  int n = val->Uint32Value();
  if (n >= 0) {
    Context2d *context = ObjectWrap::Unwrap<Context2d>(info.This());
    context->state->shadowBlur = n;
  }
}

/*
 * Get current antialiasing setting.
 */

Handle<Value>
Context2d::GetAntiAlias(Local<String> prop, const AccessorInfo &info) {
  Context2d *context = ObjectWrap::Unwrap<Context2d>(info.This());
  switch (cairo_get_antialias(context->context())) {
    case CAIRO_ANTIALIAS_NONE:
      return String::NewSymbol("none");
    case CAIRO_ANTIALIAS_GRAY:
      return String::NewSymbol("gray");
    case CAIRO_ANTIALIAS_SUBPIXEL:
      return String::NewSymbol("subpixel");
    default:
      return String::NewSymbol("default");
  }
}

/*
 * Set antialiasing.
 */

void
Context2d::SetAntiAlias(Local<String> prop, Local<Value> val, const AccessorInfo &info) {
  String::AsciiValue str(val->ToString());
  Context2d *context = ObjectWrap::Unwrap<Context2d>(info.This());
  cairo_t *ctx = context->context();
  cairo_antialias_t a;
  if (0 == strcmp("none", *str)) {
    a = CAIRO_ANTIALIAS_NONE;
  } else if (0 == strcmp("default", *str)) {
    a = CAIRO_ANTIALIAS_DEFAULT;
  } else if (0 == strcmp("gray", *str)) {
    a = CAIRO_ANTIALIAS_GRAY;
  } else if (0 == strcmp("subpixel", *str)) {
    a = CAIRO_ANTIALIAS_SUBPIXEL;
  } else {
    a = cairo_get_antialias(ctx);
  }
  cairo_set_antialias(ctx, a);
}

/*
 * Get miter limit.
 */

Handle<Value>
Context2d::GetMiterLimit(Local<String> prop, const AccessorInfo &info) {
  Context2d *context = ObjectWrap::Unwrap<Context2d>(info.This());
  return Number::New(cairo_get_miter_limit(context->context()));
}

/*
 * Set miter limit.
 */

void
Context2d::SetMiterLimit(Local<String> prop, Local<Value> val, const AccessorInfo &info) {
  double n = val->NumberValue();
  if (n > 0) {
    Context2d *context = ObjectWrap::Unwrap<Context2d>(info.This());
    cairo_set_miter_limit(context->context(), n);
  }
}

/*
 * Get line width.
 */

Handle<Value>
Context2d::GetLineWidth(Local<String> prop, const AccessorInfo &info) {
  Context2d *context = ObjectWrap::Unwrap<Context2d>(info.This());
  return Number::New(cairo_get_line_width(context->context()));
}

/*
 * Set line width.
 */

void
Context2d::SetLineWidth(Local<String> prop, Local<Value> val, const AccessorInfo &info) {
  double n = val->NumberValue();
  if (n > 0) {
    Context2d *context = ObjectWrap::Unwrap<Context2d>(info.This());
    cairo_set_line_width(context->context(), n);
  }
}

/*
 * Get line join.
 */

Handle<Value>
Context2d::GetLineJoin(Local<String> prop, const AccessorInfo &info) {
  Context2d *context = ObjectWrap::Unwrap<Context2d>(info.This());
  switch (cairo_get_line_join(context->context())) {
    case CAIRO_LINE_JOIN_BEVEL:
      return String::NewSymbol("bevel");
    case CAIRO_LINE_JOIN_ROUND:
      return String::NewSymbol("round");
    default:
      return String::NewSymbol("miter");
  }
}

/*
 * Set line join.
 */

void
Context2d::SetLineJoin(Local<String> prop, Local<Value> val, const AccessorInfo &info) {
  Context2d *context = ObjectWrap::Unwrap<Context2d>(info.This());
  cairo_t *ctx = context->context();
  String::AsciiValue type(val->ToString());
  if (0 == strcmp("round", *type)) {
    cairo_set_line_join(ctx, CAIRO_LINE_JOIN_ROUND);
  } else if (0 == strcmp("bevel", *type)) {
    cairo_set_line_join(ctx, CAIRO_LINE_JOIN_BEVEL);
  } else {
    cairo_set_line_join(ctx, CAIRO_LINE_JOIN_MITER);
  }
}

/*
 * Get line cap.
 */

Handle<Value>
Context2d::GetLineCap(Local<String> prop, const AccessorInfo &info) {
  Context2d *context = ObjectWrap::Unwrap<Context2d>(info.This());
  switch (cairo_get_line_cap(context->context())) {
    case CAIRO_LINE_CAP_ROUND:
      return String::NewSymbol("round");
    case CAIRO_LINE_CAP_SQUARE:
      return String::NewSymbol("square");
    default:
      return String::NewSymbol("butt");
  }
}

/*
 * Set line cap.
 */

void
Context2d::SetLineCap(Local<String> prop, Local<Value> val, const AccessorInfo &info) {
  Context2d *context = ObjectWrap::Unwrap<Context2d>(info.This());
  cairo_t *ctx = context->context();
  String::AsciiValue type(val->ToString());
  if (0 == strcmp("round", *type)) {
    cairo_set_line_cap(ctx, CAIRO_LINE_CAP_ROUND);
  } else if (0 == strcmp("square", *type)) {
    cairo_set_line_cap(ctx, CAIRO_LINE_CAP_SQUARE);
  } else {
    cairo_set_line_cap(ctx, CAIRO_LINE_CAP_BUTT);
  }
}

/*
 * Check if the given point is within the current path.
 */

Handle<Value>
Context2d::IsPointInPath(const Arguments &args) {
  HandleScope scope;
  if (args[0]->IsNumber() && args[1]->IsNumber()) {
    Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
    cairo_t *ctx = context->context();
    double x = args[0]->NumberValue()
         , y = args[1]->NumberValue();
    return Boolean::New(cairo_in_fill(ctx, x, y) || cairo_in_stroke(ctx, x, y));
  }
  return False();
}

/*
 * Set fill pattern, used internally for fillStyle=
 */

Handle<Value>
Context2d::SetFillPattern(const Arguments &args) {
  HandleScope scope;

  Local<Object> obj = args[0]->ToObject();
  if (!Gradient::constructor->HasInstance(obj))
    return ThrowException(Exception::TypeError(String::New("Gradient expected")));

  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  Gradient *grad = ObjectWrap::Unwrap<Gradient>(obj);
  context->state->fillPattern = grad->pattern();
  return Undefined();
}

/*
 * Set stroke pattern, used internally for strokeStyle=
 */

Handle<Value>
Context2d::SetStrokePattern(const Arguments &args) {
  HandleScope scope;

  Local<Object> obj = args[0]->ToObject();
  if (!Gradient::constructor->HasInstance(obj))
    return ThrowException(Exception::TypeError(String::New("Gradient expected")));

  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  Gradient *grad = ObjectWrap::Unwrap<Gradient>(obj);
  context->state->strokePattern = grad->pattern();
  return Undefined();
}

/*
 * Set shadow color.
 */

void
Context2d::SetShadowColor(Local<String> prop, Local<Value> val, const AccessorInfo &info) {
  short ok;
  String::AsciiValue str(val->ToString());
  uint32_t rgba = rgba_from_string(*str, &ok);
  if (ok) {
    Context2d *context = ObjectWrap::Unwrap<Context2d>(info.This());
    context->state->shadow = rgba_create(rgba);
  }
}

/*
 * Get shadow color.
 */

Handle<Value>
Context2d::GetShadowColor(Local<String> prop, const AccessorInfo &info) {
  char buf[64];
  Context2d *context = ObjectWrap::Unwrap<Context2d>(info.This());
  rgba_to_string(context->state->shadow, buf);
  return String::New(buf);
}

/*
 * Set fill color, used internally for fillStyle=
 */

Handle<Value>
Context2d::SetFillColor(const Arguments &args) {
  HandleScope scope;
  short ok;
  if (!args[0]->IsString()) return Undefined();
  String::AsciiValue str(args[0]);
  uint32_t rgba = rgba_from_string(*str, &ok);
  if (!ok) return Undefined();
  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  context->state->fillPattern = NULL;
  context->state->fill = rgba_create(rgba);
  return Undefined();
}

/*
 * Get fill color.
 */

Handle<Value>
Context2d::GetFillColor(Local<String> prop, const AccessorInfo &info) {
  char buf[64];
  Context2d *context = ObjectWrap::Unwrap<Context2d>(info.This());
  rgba_to_string(context->state->fill, buf);
  return String::New(buf);
}

/*
 * Set stroke color, used internally for strokeStyle=
 */

Handle<Value>
Context2d::SetStrokeColor(const Arguments &args) {
  HandleScope scope;
  short ok;
  if (!args[0]->IsString()) return Undefined();
  String::AsciiValue str(args[0]);
  uint32_t rgba = rgba_from_string(*str, &ok);
  if (!ok) return Undefined();
  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  context->state->strokePattern = NULL;
  context->state->stroke = rgba_create(rgba);
  return Undefined();
}

/*
 * Get stroke color.
 */

Handle<Value>
Context2d::GetStrokeColor(Local<String> prop, const AccessorInfo &info) {
  char buf[64];
  Context2d *context = ObjectWrap::Unwrap<Context2d>(info.This());
  rgba_to_string(context->state->stroke, buf);
  return String::New(buf);
}

/*
 * Bezier curve.
 */

Handle<Value>
Context2d::BezierCurveTo(const Arguments &args) {
  HandleScope scope;

  if (!args[0]->IsNumber()
    ||!args[1]->IsNumber()
    ||!args[2]->IsNumber()
    ||!args[3]->IsNumber()
    ||!args[4]->IsNumber()
    ||!args[5]->IsNumber()) return Undefined();

  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  cairo_curve_to(context->context()
    , args[0]->NumberValue()
    , args[1]->NumberValue()
    , args[2]->NumberValue()
    , args[3]->NumberValue()
    , args[4]->NumberValue()
    , args[5]->NumberValue());

  return Undefined();
}

/*
 * Quadratic curve approximation from libsvg-cairo.
 */

Handle<Value>
Context2d::QuadraticCurveTo(const Arguments &args) {
  HandleScope scope;

  if (!args[0]->IsNumber()
    ||!args[1]->IsNumber()
    ||!args[2]->IsNumber()
    ||!args[3]->IsNumber()) return Undefined();

  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  cairo_t *ctx = context->context();

  double x, y
    , x1 = args[0]->NumberValue()
    , y1 = args[1]->NumberValue()
    , x2 = args[2]->NumberValue()
    , y2 = args[3]->NumberValue();

  cairo_get_current_point(ctx, &x, &y);

  cairo_curve_to(ctx
    , x  + 2.0 / 3.0 * (x1 - x),  y  + 2.0 / 3.0 * (y1 - y)
    , x2 + 2.0 / 3.0 * (x1 - x2), y2 + 2.0 / 3.0 * (y1 - y2)
    , x2
    , y2);

  return Undefined();
}

/*
 * Save state.
 */

Handle<Value>
Context2d::Save(const Arguments &args) {
  HandleScope scope;
  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  context->save();
  return Undefined();
}

/*
 * Restore state.
 */

Handle<Value>
Context2d::Restore(const Arguments &args) {
  HandleScope scope;
  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  context->restore();
  return Undefined();
}

/*
 * Creates a new subpath.
 */

Handle<Value>
Context2d::BeginPath(const Arguments &args) {
  HandleScope scope;
  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  cairo_new_path(context->context());
  return Undefined();
}

/*
 * Marks the subpath as closed.
 */

Handle<Value>
Context2d::ClosePath(const Arguments &args) {
  HandleScope scope;
  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  cairo_close_path(context->context());
  return Undefined();
}

/*
 * Rotate transformation.
 */

Handle<Value>
Context2d::Rotate(const Arguments &args) {
  HandleScope scope;
  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  cairo_rotate(context->context()
    , args[0]->IsNumber() ? args[0]->NumberValue() : 0);
  return Undefined();
}

/*
 * Modify the CTM.
 */

Handle<Value>
Context2d::Transform(const Arguments &args) {
  HandleScope scope;

  cairo_matrix_t matrix;
  cairo_matrix_init(&matrix
    , args[0]->IsNumber() ? args[0]->NumberValue() : 0
    , args[1]->IsNumber() ? args[1]->NumberValue() : 0
    , args[2]->IsNumber() ? args[2]->NumberValue() : 0
    , args[3]->IsNumber() ? args[3]->NumberValue() : 0
    , args[4]->IsNumber() ? args[4]->NumberValue() : 0
    , args[5]->IsNumber() ? args[5]->NumberValue() : 0);

  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  cairo_transform(context->context(), &matrix);
  
  return Undefined();
}

/*
 * Reset the CTM, used internally by setTransform().
 */

Handle<Value>
Context2d::ResetTransform(const Arguments &args) {
  HandleScope scope;
  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  cairo_identity_matrix(context->context());
  return Undefined();
}

/*
 * Translate transformation.
 */

Handle<Value>
Context2d::Translate(const Arguments &args) {
  HandleScope scope;
  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  cairo_translate(context->context()
    , args[0]->IsNumber() ? args[0]->NumberValue() : 0
    , args[1]->IsNumber() ? args[1]->NumberValue() : 0);
  return Undefined();
}

/*
 * Scale transformation.
 */

Handle<Value>
Context2d::Scale(const Arguments &args) {
  HandleScope scope;
  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  cairo_scale(context->context()
    , args[0]->IsNumber() ? args[0]->NumberValue() : 0
    , args[1]->IsNumber() ? args[1]->NumberValue() : 0);
  return Undefined();
}

/*
 * Use path as clipping region.
 */

Handle<Value>
Context2d::Clip(const Arguments &args) {
  HandleScope scope;
  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  cairo_t *ctx = context->context();
  cairo_clip_preserve(ctx);
  return Undefined();
}

/*
 * Fill the path.
 */

Handle<Value>
Context2d::Fill(const Arguments &args) {
  HandleScope scope;
  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  context->fill(true);
  return Undefined();
}

/*
 * Stroke the path.
 */

Handle<Value>
Context2d::Stroke(const Arguments &args) {
  HandleScope scope;
  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  context->stroke(true);
  return Undefined();
}

/*
 * Fill text at (x, y).
 */

Handle<Value>
Context2d::FillText(const Arguments &args) {
  HandleScope scope;

  if (!args[0]->IsString()
    || !args[1]->IsNumber()
    || !args[2]->IsNumber()) return Undefined();

  String::Utf8Value str(args[0]);
  double x = args[1]->NumberValue();
  double y = args[2]->NumberValue();

  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());

  context->savePath();
  context->setTextPath(*str, x, y);
  context->fill();
  context->restorePath();

  return Undefined();
}

/*
 * Stroke text at (x ,y).
 */

Handle<Value>
Context2d::StrokeText(const Arguments &args) {
  HandleScope scope;
  
  if (!args[0]->IsString()
    || !args[1]->IsNumber()
    || !args[2]->IsNumber()) return Undefined();

  String::Utf8Value str(args[0]);
  double x = args[1]->NumberValue();
  double y = args[2]->NumberValue();
  
  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());

  context->savePath();
  context->setTextPath(*str, x, y);
  context->stroke();
  context->restorePath();

  return Undefined();
}

/*
 * Set text path for the given string at (x, y).
 */

void
Context2d::setTextPath(const char *str, double x, double y) {
  // Text extents
  cairo_text_extents_t te;
  cairo_text_extents(_context, str, &te);

  cairo_font_extents_t fe;
  cairo_font_extents(_context, &fe);

  // Alignment
  switch (state->textAlignment) {
    // center
    case 0:
      x -= te.width / 2 + te.x_bearing;
      break;
    // right
    case 1:
      x -= te.width + te.x_bearing;
      break;
  }

  // Baseline approx
  // TODO:
  switch (state->textBaseline) {
    case TEXT_BASELINE_TOP:
    case TEXT_BASELINE_HANGING:
      y += te.height;
      break;
    case TEXT_BASELINE_MIDDLE:
      y += te.height / 2;
      break;
    case TEXT_BASELINE_BOTTOM:
      y -= te.height / 2;
      break;
  }

  cairo_move_to(_context, x, y);
  cairo_text_path(_context, str);
}

/*
 * Adds a point to the current subpath.
 */

Handle<Value>
Context2d::LineTo(const Arguments &args) {
  HandleScope scope;

  if (!args[0]->IsNumber()) 
    return ThrowException(Exception::TypeError(String::New("x required")));
  if (!args[1]->IsNumber()) 
    return ThrowException(Exception::TypeError(String::New("y required")));

  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  cairo_line_to(context->context()
    , args[0]->NumberValue()
    , args[1]->NumberValue());

  return Undefined();
}

/*
 * Creates a new subpath at the given point.
 */

Handle<Value>
Context2d::MoveTo(const Arguments &args) {
  HandleScope scope;

  if (!args[0]->IsNumber()) 
    return ThrowException(Exception::TypeError(String::New("x required")));
  if (!args[1]->IsNumber()) 
    return ThrowException(Exception::TypeError(String::New("y required")));

  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  cairo_move_to(context->context()
    , args[0]->NumberValue()
    , args[1]->NumberValue());

  return Undefined();
}

/*
 * Set font:
 *   - weight
 *   - style
 *   - size
 *   - unit
 *   - family
 */

Handle<Value>
Context2d::SetFont(const Arguments &args) {
  HandleScope scope;

  // Ignore invalid args
  if (!args[0]->IsString()
    || !args[1]->IsString()
    || !args[2]->IsNumber()
    || !args[3]->IsString()
    || !args[4]->IsString()) return Undefined();

  String::AsciiValue weight(args[0]);
  String::AsciiValue style(args[1]);
  double size = args[2]->NumberValue();
  String::AsciiValue unit(args[3]);
  const char *family = *String::AsciiValue(args[4]);
  
  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  cairo_t *ctx = context->context();

  // Size
  cairo_set_font_size(ctx, size);

  // Family
  if (0 == strcmp("sans-serif", family)) {
    family = "Arial";
  }

  // Style
  cairo_font_slant_t s = CAIRO_FONT_SLANT_NORMAL;
  if (0 == strcmp("italic", *style)) {
    s = CAIRO_FONT_SLANT_ITALIC;
  } else if (0 == strcmp("oblique", *style)) {
    s = CAIRO_FONT_SLANT_OBLIQUE;
  }

  // Weight
  cairo_font_weight_t w = CAIRO_FONT_WEIGHT_NORMAL;
  if (0 == strcmp("bold", *weight)) {
    w = CAIRO_FONT_WEIGHT_BOLD;
  }

  cairo_select_font_face(ctx, family, s, w);
  
  return Undefined();
}

/*
 * Return the given text extents.
 */

Handle<Value>
Context2d::MeasureText(const Arguments &args) {
  HandleScope scope;

  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  cairo_t *ctx = context->context();

  String::Utf8Value str(args[0]->ToString());
  Local<Object> obj = Object::New();

  cairo_text_extents_t te;
  cairo_text_extents(ctx, *str, &te);
  obj->Set(String::New("width"), Number::New(te.width));

  return scope.Close(obj);
}

/*
 * Set text baseline.
 */

Handle<Value>
Context2d::SetTextBaseline(const Arguments &args) {
  HandleScope scope;

  if (!args[0]->IsInt32()) return Undefined();
  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  context->state->textBaseline = args[0]->Int32Value();

  return Undefined();
}

/*
 * Set text alignment. -1 0 1
 */

Handle<Value>
Context2d::SetTextAlignment(const Arguments &args) {
  HandleScope scope;

  if (!args[0]->IsInt32()) return Undefined();
  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  context->state->textAlignment = args[0]->Int32Value();

  return Undefined();
}

/*
 * Fill the rectangle defined by x, y, width and height.
 */

Handle<Value>
Context2d::FillRect(const Arguments &args) {
  HandleScope scope;
  RECT_ARGS;
  if (0 == width || 0 == height) return Undefined();
  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  cairo_t *ctx = context->context();
  cairo_new_path(ctx);
  cairo_rectangle(ctx, x, y, width, height);
  context->fill();
  return Undefined();
}

/*
 * Stroke the rectangle defined by x, y, width and height.
 */

Handle<Value>
Context2d::StrokeRect(const Arguments &args) {
  HandleScope scope;
  RECT_ARGS;
  if (0 == width && 0 == height) return Undefined();
  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  cairo_t *ctx = context->context();
  cairo_new_path(ctx);
  cairo_rectangle(ctx, x, y, width, height);
  context->stroke();
  return Undefined();
}

/*
 * Clears all pixels defined by x, y, width and height.
 */

Handle<Value>
Context2d::ClearRect(const Arguments &args) {
  HandleScope scope;
  RECT_ARGS;
  if (0 == width || 0 == height) return Undefined();
  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  cairo_t *ctx = context->context();
  cairo_save(ctx);
  cairo_rectangle(ctx, x, y, width, height);
  cairo_set_operator(ctx, CAIRO_OPERATOR_CLEAR);
  cairo_fill(ctx);
  cairo_restore(ctx);
  return Undefined();
}

/*
 * Adds a rectangle subpath.
 */

Handle<Value>
Context2d::Rect(const Arguments &args) {
  HandleScope scope;
  RECT_ARGS;
  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  cairo_rectangle(context->context(), x, y, width, height);
  return Undefined();
}

/*
 * Adds an arc at x, y with the given radis and start/end angles.
 */

Handle<Value>
Context2d::Arc(const Arguments &args) {
  HandleScope scope;

  if (!args[0]->IsNumber()
    || !args[1]->IsNumber()
    || !args[2]->IsNumber()
    || !args[3]->IsNumber()
    || !args[4]->IsNumber()) return Undefined();

  bool anticlockwise = args[5]->BooleanValue();

  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  cairo_t *ctx = context->context();

  if (anticlockwise && M_PI * 2 != args[4]->NumberValue()) {
    cairo_arc_negative(ctx
      , args[0]->NumberValue()
      , args[1]->NumberValue()
      , args[2]->NumberValue()
      , args[3]->NumberValue()
      , args[4]->NumberValue());
  } else {
    cairo_arc(ctx
      , args[0]->NumberValue()
      , args[1]->NumberValue()
      , args[2]->NumberValue()
      , args[3]->NumberValue()
      , args[4]->NumberValue());
  }

  return Undefined();
}

/*
 * Adds an arcTo point (x0,y0) to (x1,y1) with the given radius.
 *
 * Implementation influenced by WebKit.
 */

Handle<Value>
Context2d::ArcTo(const Arguments &args) {
  HandleScope scope;

  if (!args[0]->IsNumber()
    || !args[1]->IsNumber()
    || !args[2]->IsNumber()
    || !args[3]->IsNumber()
    || !args[4]->IsNumber()) return Undefined();

  Context2d *context = ObjectWrap::Unwrap<Context2d>(args.This());
  cairo_t *ctx = context->context();

  // Current path point
  double x, y;
  cairo_get_current_point(ctx, &x, &y);
  Point<float> p0(x, y);

  // Point (x0,y0)
  Point<float> p1(args[0]->NumberValue(), args[1]->NumberValue());

  // Point (x1,y1)
  Point<float> p2(args[2]->NumberValue(), args[3]->NumberValue());

  float radius = args[4]->NumberValue();

  if ((p1.x == p0.x && p1.y == p0.y)
    || (p1.x == p2.x && p1.y == p2.y)
    || radius == 0.f) {
    cairo_line_to(ctx, p1.x, p1.y);
    return Undefined();
  }

  Point<float> p1p0((p0.x - p1.x),(p0.y - p1.y));
  Point<float> p1p2((p2.x - p1.x),(p2.y - p1.y));
  float p1p0_length = sqrtf(p1p0.x * p1p0.x + p1p0.y * p1p0.y);
  float p1p2_length = sqrtf(p1p2.x * p1p2.x + p1p2.y * p1p2.y);

  double cos_phi = (p1p0.x * p1p2.x + p1p0.y * p1p2.y) / (p1p0_length * p1p2_length);
  // all points on a line logic
  if (-1 == cos_phi) {
    cairo_line_to(ctx, p1.x, p1.y);
    return Undefined();
  }

  if (1 == cos_phi) {
    // add infinite far away point
    unsigned int max_length = 65535;
    double factor_max = max_length / p1p0_length;
    Point<float> ep((p0.x + factor_max * p1p0.x), (p0.y + factor_max * p1p0.y));
    cairo_line_to(ctx, ep.x, ep.y);
    return Undefined();
  }

  float tangent = radius / tan(acos(cos_phi) / 2);
  float factor_p1p0 = tangent / p1p0_length;
  Point<float> t_p1p0((p1.x + factor_p1p0 * p1p0.x), (p1.y + factor_p1p0 * p1p0.y));

  Point<float> orth_p1p0(p1p0.y, -p1p0.x);
  float orth_p1p0_length = sqrt(orth_p1p0.x * orth_p1p0.x + orth_p1p0.y * orth_p1p0.y);
  float factor_ra = radius / orth_p1p0_length;

  double cos_alpha = (orth_p1p0.x * p1p2.x + orth_p1p0.y * p1p2.y) / (orth_p1p0_length * p1p2_length);
  if (cos_alpha < 0.f)
      orth_p1p0 = Point<float>(-orth_p1p0.x, -orth_p1p0.y);

  Point<float> p((t_p1p0.x + factor_ra * orth_p1p0.x), (t_p1p0.y + factor_ra * orth_p1p0.y));

  orth_p1p0 = Point<float>(-orth_p1p0.x, -orth_p1p0.y);
  float sa = acos(orth_p1p0.x / orth_p1p0_length);
  if (orth_p1p0.y < 0.f)
      sa = 2 * M_PI - sa;

  bool anticlockwise = false;

  float factor_p1p2 = tangent / p1p2_length;
  Point<float> t_p1p2((p1.x + factor_p1p2 * p1p2.x), (p1.y + factor_p1p2 * p1p2.y));
  Point<float> orth_p1p2((t_p1p2.x - p.x),(t_p1p2.y - p.y));
  float orth_p1p2_length = sqrtf(orth_p1p2.x * orth_p1p2.x + orth_p1p2.y * orth_p1p2.y);
  float ea = acos(orth_p1p2.x / orth_p1p2_length);

  if (orth_p1p2.y < 0) ea = 2 * M_PI - ea;
  if ((sa > ea) && ((sa - ea) < M_PI)) anticlockwise = true;
  if ((sa < ea) && ((ea - sa) > M_PI)) anticlockwise = true;

  cairo_line_to(ctx, t_p1p0.x, t_p1p0.y);

  if (anticlockwise && M_PI * 2 != radius) {
    cairo_arc_negative(ctx
      , p.x
      , p.y
      , radius
      , sa
      , ea);
  } else {
    cairo_arc(ctx
      , p.x
      , p.y
      , radius
      , sa
      , ea);
  }

  return Undefined();
}
