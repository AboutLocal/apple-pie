fs = require('fs')
sys = require('sys')
http = require('http')
d3=require('d3')
url = require('url')
CanvasSvg = require('./lib/node-canvas-svg/lib/canvas-svg')
chartCreator=require("./createSphere.coffee")

#Container vorbereiten, in dem gemalt werden kann
d3.select("body")
.append("div")
.attr("id", "chart")
.append("svg")
.attr("id", "background")
.attr("width", 750)
.attr("height", 750)


#gibt den svg Code für ein PieChart mit percent % zurück
getSvgText= (percent,width,height,radius) ->
	window.document.getElementById("chart").innerHTML="";
	chartCreator.createPieChart("#chart", percent,width,height,radius)
	svgText=window.document.getElementById("chart").childNodes[0].outerHTML
	svgText="""<?xml version="1.0" standalone="yes"?>
	<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN"
	"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">"""+svgText
	svgText=svgText.split('lineargradient').join('linearGradient')
	return svgText

#gibt paramValue zurück, oder default, wenn pramValue keine Zahl >= 0 ist
verifyNumber= (paramValue, defaultValue) ->
	value=paramValue ? defaultValue
	if isNaN(value) then value=defaultValue
	if value<=0 then value=defaultValue
	return value



#Server starten, der eine grafische Darstellung der angefragten Prozentzahl zurückgibt
http=require 'http'
server=http.createServer (req, res) ->
	res.writeHead 200, {'Content-Type': 'image.png'}

	CanvasSvg.load (err) ->
		if (err) then throw err
		
		#Parameter parsen
		params=url.parse(req.url, true).query

		percent=params.percent ? 0
		if isNaN(percent) then percent=0
		if percent<0 then percent=0
		if percent>100 then percent=100
		
		width=verifyNumber(params.width, 450)
		height=verifyNumber(params.height, 300)
		radius=verifyNumber(params.radius, 43)
		
		svgText=getSvgText(percent,width,height,radius)
		CanvasSvg.svg.render svgText, (err, canvas) ->
			if (err) then throw err
			png = canvas.createPNGStream()
			png.on('data', (data) ->
				res.write(data)
			)
			png.on('end', () ->
				res.end()
			)
	
server.listen 1337, '127.0.0.1'
console.log 'piePng Server running at http://127.0.0.1:1337/'

