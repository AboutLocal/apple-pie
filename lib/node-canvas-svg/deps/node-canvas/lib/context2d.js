
/*!
 * Canvas - Context2d
 * Copyright (c) 2010 LearnBoost <tj@learnboost.com>
 * MIT Licensed
 */

/**
 * Module dependencies.
 */

var canvas = require('../build/Release/canvas')
  , Context2d = canvas.CanvasRenderingContext2d
  , CanvasGradient = canvas.CanvasGradient
  , ImageData = canvas.ImageData
  , PixelArray = canvas.CanvasPixelArray;

/**
 * Export `Context2d` as the module.
 */

var Context2d = exports = module.exports = Context2d;

/**
 * Cache color string RGBA values.
 */

var cache = {};

/**
 * Text baselines.
 */

var baselines = ['alphabetic', 'top', 'bottom', 'middle', 'ideographic', 'hanging'];

/**
 * Font RegExp helpers.
 */

var weights = 'normal|bold|bolder|lighter|[1-9]00'
  , styles = 'normal|italic|oblique'
  , units = 'px|pt|pc|in|cm|mm|%'
  , string = '\'([^\']+)\'|"([^"]+)"|[\\w-]+';

/**
 * Font parser RegExp;
 */

var fontre = new RegExp('^ *'
  + '(?:(' + weights + ') *)?'
  + '(?:(' + styles + ') *)?'
  + '([\\d\\.]+)(' + units + ') *'
  + '((?:' + string + ')( *, *(?:' + string + '))*)'
  );

/**
 * Parse font `str`.
 *
 * @param {String} str
 * @return {Object}
 * @api private
 */

var parseFont = exports.parseFont = function(str){
  var font = {}
    , captures = fontre.exec(str);

  // Invalid
  if (!captures) return;

  // Populate font object
  font.weight = captures[1] || 'normal';
  font.style = captures[2] || 'normal';
  font.size = parseFloat(captures[3]);
  font.unit = captures[4];
  font.family = captures[5];

  return font;
};

/**
 * Create a linear gradient at the given point `(x0, y0)` and `(x1, y1)`.
 *
 * @param {Number} x0
 * @param {Number} y0
 * @param {Number} x1
 * @param {Number} y1
 * @return {CanvasGradient}
 * @api public
 */

Context2d.prototype.createLinearGradient = function(x0, y0, x1, y1){
  return new CanvasGradient(x0, y0, x1, y1);
};

/**
 * Create a radial gradient at the given point `(x0, y0)` and `(x1, y1)`
 * and radius `r0` and `r1`.
 *
 * @param {Number} x0
 * @param {Number} y0
 * @param {Number} r0
 * @param {Number} x1
 * @param {Number} y1
 * @param {Number} r1
 * @return {CanvasGradient}
 * @api public
 */

Context2d.prototype.createRadialGradient = function(x0, y0, r0, x1, y1, r1){
  return new CanvasGradient(x0, y0, r0, x1, y1, r1);
};

/**
 * Reset transform matrix to identity, then apply the given args.
 *
 * @param {...}
 * @api public
 */

Context2d.prototype.setTransform = function(){
  this.resetTransform();
  this.transform.apply(this, arguments);
};

/**
 * Set the fill style with the given css color string.
 *
 * @api public
 */

Context2d.prototype.__defineSetter__('fillStyle', function(val){
  if (val instanceof CanvasGradient) {
    this.lastFillStyle = val;
    this.setFillPattern(val);
  } else if ('string' == typeof val) {
    this.setFillColor(val);
  }
});

/**
 * Get previous fill style.
 *
 * @return {CanvasGradient|String}
 * @api public
 */

Context2d.prototype.__defineGetter__('fillStyle', function(){
  return this.lastFillStyle || this.fillColor;
});

/**
 * Set the stroke style with the given css color string.
 *
 * @api public
 */

Context2d.prototype.__defineSetter__('strokeStyle', function(val){
  if (val instanceof CanvasGradient) {
    this.lastStrokeStyle = val;
    this.setStrokePattern(val);
  } else if ('string' == typeof val) {
    this.setStrokeColor(val);
  }
});

/**
 * Get previous stroke style.
 *
 * @return {CanvasGradient|String}
 * @api public
 */

Context2d.prototype.__defineGetter__('strokeStyle', function(){
  return this.lastStrokeStyle || this.strokeColor;
});


/**
 * Set font.
 *
 * @see exports.parseFont()
 * @api public
 */

Context2d.prototype.__defineSetter__('font', function(val){
  if ('string' == typeof val) {
    var font;
    if (font = cache[val] || parseFont(val)) {
      this.lastFontString = val;

      // TODO: dpi
      // TODO: remaining unit conversion
      switch (font.unit) {
        case 'pt':
          font.size /= .75;
          break;
        case 'in':
          font.size *= 96;
          break;
        case 'mm':
          font.size *= 96.0 / 25.4;
          break;
        case 'cm':
          font.size *= 96.0 / 2.54;
          break;
      }

      // Cache font object
      cache[val] = font;

      // Set font
      this.setFont(
          font.weight
        , font.style
        , font.size
        , font.unit
        , font.family);
    }
  }
});

/**
 * Get the current font.
 *
 * @api public
 */

Context2d.prototype.__defineGetter__('font', function(){
  return this.lastFontString || '10px sans-serif';
});

/**
 * Set text baseline.
 *
 * @api public
 */

Context2d.prototype.__defineSetter__('textBaseline', function(val){
  var n = baselines.indexOf(val);
  if (~n) {
    this.lastBaseline = val;
    this.setTextBaseline(n);
  }
});

/**
 * Get the current baseline setting.
 *
 * @api public
 */

Context2d.prototype.__defineGetter__('textAlign', function(){
  return this.lastBaseline || 'alphabetic';
});

/**
 * Set text alignment.
 *
 * @api public
 */

Context2d.prototype.__defineSetter__('textAlign', function(val){
  switch (val) {
    case 'center':
      this.setTextAlignment(0);
      this.lastTextAlignment = val;
      break;
    case 'left':
    case 'start':
      this.setTextAlignment(-1);
      this.lastTextAlignment = val;
      break;
    case 'right':
    case 'end':
      this.setTextAlignment(1);
      this.lastTextAlignment = val;
      break;
  }
});

/**
 * Get the current font.
 *
 * @see exports.parseFont()
 * @api public
 */

Context2d.prototype.__defineGetter__('textAlign', function(){
  return this.lastTextAlignment || 'start';
});

/**
 * Get `ImageData` with the given rect.
 *
 * @param {Number} x
 * @param {Number} y
 * @param {Number} width
 * @param {Number} height
 * @return {ImageData}
 * @api public
 */

Context2d.prototype.getImageData = function(x, y, width, height){
  var arr = new PixelArray(this.canvas, x, y, width, height);
  return new ImageData(arr);
};

/**
 * Create `ImageData` with the given dimensions or
 * `ImageData` instance for dimensions.
 *
 * @param {Number|ImageData} width
 * @param {Number} height
 * @return {ImageData}
 * @api public
 */

Context2d.prototype.createImageData = function(width, height){
  if (width instanceof ImageData) {
    height = width.height;
    width = width.width;
  }
  return new ImageData(new PixelArray(width, height));
};
