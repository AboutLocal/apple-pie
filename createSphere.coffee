this.createPieChart = (containerSelector, percentValue, width, height, outerRadius) ->

    # config
#    w = 450
#    h = 300
#    r = 43
    w=width
    h=height
    r=outerRadius
    ir = 0
    textOffset = 14
    tweenDuration = 250

    innerCircleRadius = r - 8
    strokeWidth = 1

    # data objects
    #lines, valueLabels, nameLabels
    pieData = []
    oldPieData = []
    filteredPieData = []

    # pie helper
    pie = d3.layout.pie().value (d) ->
      d.value

    # arc helper
    arc = d3.svg.arc()
    arc.startAngle (d) -> d.startAngle
    arc.endAngle (d) -> d.endAngle
    arc.innerRadius ir
    arc.outerRadius innerCircleRadius - strokeWidth

    # small arc helper
    smallArc = d3.svg.arc()
    smallArc.startAngle (d) -> d.startAngle
    smallArc.endAngle (d) -> d.endAngle
    smallArc.innerRadius ir
    smallArc.outerRadius innerCircleRadius - strokeWidth * 2

    vis = d3.select(containerSelector).append("svg:svg").attr("width", w).attr("height", h)

    arcGroup = vis.append("svg:g")
      .attr("class", "arc")
      .attr("transform", "translate(" + (w/2) + "," + (h/2) + ")")

    greenGradient = arcGroup.append("svg:linearGradient")
      .attr("id", "greenGradient")
      .attr("x1", "0%")
      .attr("x2", "0%")
      .attr("y1", "0%")
      .attr("y2", "100%")

    greenGradient.append("svg:stop")
      .attr("offset", "0%")
      #.attr("style", "stop-color:#84c917;stop-opacity:1")
      .attr("stop-color", "#84c917")
      #.attr("stop-opacity", "1")

    greenGradient.append("svg:stop")
      .attr("offset", "100%")
      #.attr("style", "stop-color:#6fa817;stop-opacity:1")
      .attr("stop-color", "#6fa817")
      #.attr("stop-opacity", "1")

    reliefGradient = arcGroup.append("svg:linearGradient")
      .attr("id", "reliefGradient")
      .attr("x1", "0%")
      .attr("x2", "0%")
      .attr("y1", "0%")
      .attr("y2", "100%")

    reliefGradient.append("svg:stop")
      .attr("offset", "0%")
      .attr("style", "stop-color:#9bd343;stop-opacity:1")

    reliefGradient.append("svg:stop")
      .attr("offset", "100%")
      .attr("style", "stop-color:#9bd343;stop-opacity:0")

    labelGroup = vis.append("svg:g")
      .attr("class", "labelGroup")
      .attr("transform", "translate(" + (w/2) + "," + (h/2) + ")")

    centerGroup = vis.append("svg:g")
      .attr("class", "centerGroup")
      .attr("transform", "translate(" + (w/2) + "," + (h/2) + ")")

    outerCircle = arcGroup.append("svg:circle")
      .attr("fill", "#eaebeb")
      .attr("r", r)

    innerCircle = arcGroup.append("svg:circle")
      .attr("fill", "#ced3d7")
      .attr("r", innerCircleRadius)

    endRadians = percentValue*3.6/180*Math.PI

    arcGroup.append("svg:path")
      .attr("fill", "url(#greenGradient)")
      #.attr("fill", "rgba(255,255,255,1)")
      .attr("stroke", "#72ad16")
      .attr("d", arc({startAngle: 0, endAngle: endRadians}))

    arcGroup.append("svg:path")
      #.attr("fill", "rgba(0,0,0,0)")
      .attr("fill", "rgba(255,0,255,0)")
#      .attr("fill-opacity", "0")
      .attr("stroke", "url(#reliefGradient)")
      .attr("d", smallArc({startAngle: 0, endAngle: endRadians}))



