---
layout: page
title: Documentation
permalink: /documentation/

classList:
- title: Stepper
  link: stepper/
  description: > 
    Physical properties (pins, speed...), target position

- title: StepControl
  link: stepControl/
  description: > 
    Move one or motors to their target positions

- title: RotateControl
  link: controller/
  description: > 
    Continuously rotate one or more motors

---
## Classes

<table>
{% for class in page.classList %}
  <tr>
    <td> <a href="{{ class.link }}">{{ class.title }}</a></td>
    <td>{{class.description}}</td>
  </tr>  
{% endfor %}
</table>

## Examples


## Other
* [LICENSE](LICENCE){:.heading.flip-title} 


