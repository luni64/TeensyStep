---
title: Applications
layout: myAppIndex

permalink: /applications/

applications:
    - folder: winder
      title: String Making Machine
      description: >
        How to use TeensyStep to implement  
        the controlling algorithms for a string making machine
      image:       
        title:   Christoph Weigel, der saitenmacher, Regensburg 1698
        href:    winder/assets/largeTitle.png
        

    - title: Multichannel CNC Drill
      folder: cnc_drill
      description: >
        Using TeensyStep for a DIY multichannel CNC drill
      image:       
        title:   Pieter Verbruggen, ca 1778 [Public domain], via Wikimedia Commons
        href:    https://upload.wikimedia.org/wikipedia/commons/5/57/Jan_Verbruggen_Foudary_Drawing_47_Horizontal_Boring_Machine_%28cropped%29.JPG
---

xxy

{% for page in site.application.pages%}
xx
{{ page.headline }}


{% endfor %}


