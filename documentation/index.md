---
layout: page
description: >  
    Here you find everything you need to know to use TeensyStep in your projects. 
    Should you discover a mistake in the documentation, missing explanations or a bug in general, feel free to [open an issue](https://github.com/luni64/TeensyStep/issues) on GitHub.
# hide_description: true
permalink: /documentation/
---
# Documentation
## Class List

<table>
{% for class in site.data.classList %}
  <tr>
    <td> <a href="{{ class.link }}">{{ class.title }}</a></td>
    <td>{{class.description}}</td>
  </tr>  
{% endfor %}
</table>


## Examples


## Other
* [LICENSE](LICENCE){:.heading.flip-title} 


