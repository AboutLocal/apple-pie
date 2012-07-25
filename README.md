* da node-canvas-svg require.paths verwendet, was in neueren node-Versionen nichtmehr erlaubt ist, habe ich die entsprechenden Aufrufe von require manuell auf den richtigen Ordner umgebogen
* das makefile von node-overload muss entsprechend https://github.com/bmeck/node-overload/pull/7 angepasst werden
* ./lib/node-canvas-svg/deps/node-canvas/lib/canvas.js, Zeile 12: im Pfad "default" durch "Release" ersetzen
* ./lib/node-canvas-svg/deps/node-canvas/lib/context2d.js, Zeile 12: im Pfad "default" durch "Release" ersetzen
* ./lib/node-canvas-svg/deps/node-canvas/lib/image.js, Zeile 12: im Pfad "default" durch "Release" ersetzen
* ./lib/node-canvas-svg/lib/canvas-svg/loader.js, Zeile 52: "process.compile" durch "require('vm').runInThisContext" ersetzen

* Voraussetzungen:
  * node-canvas-svg von https://github.com/dodo/node-canvas-svg

