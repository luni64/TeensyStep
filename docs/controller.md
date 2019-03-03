---
title: Controller
description: >
  This chapter covers the basics of content creation with Hydejack.
hide_description: true
---

# Basics
This chapter covers the basics of content creation with Hydejack.

## Table of Contents
{:.no_toc}

0. this unordered seed list will be replaced by toc as unordered list
{:toc}


## Adding a page
You can add generic pages that support markdown content but aren't blog posts.
For example, this documentation is written in markdown, consisting of several generic pages.

To add a page, create a new markdown file and put `layout: page` in a front matter

~~~yml
---
layout: page
title:  Documentation
---
~~~

Now you can add content as you would in a blog post.


## Adding an entry to the sidebar
To add links to the sidebar, populate the `menu` entry in `_config.yml` with a list of `title`-`url` pairs, e.g.:
You’ll find this post in your `_posts` directory. Go ahead and edit it and re-build the site to see your changes. You can rebuild the site in many different ways, but the most common way is to run `jekyll serve`, which launches a web server and auto-regenerates your site when a file is updated.

To add new posts, simply add a file in the `_posts` directory that follows the convention `YYYY-MM-DD-name-of-post.ext` and includes the necessary front matter. Take a look at the source for this post to get an idea about how it works.

Jekyll also offers powerful support for code snippets:

{% highlight c++ %}
class x
{
  void doit();
}
{% endhighlight %}

Check out the [Jekyll docs][jekyll-docs] for more info on how to get the most out of Jekyll. File all bugs/feature requests at [Jekyll’s GitHub repo][jekyll-gh]. If you have questions, you can ask them on [Jekyll Talk][jekyll-talk].

[jekyll-docs]: https://jekyllrb.com/docs/home
[jekyll-gh]:   https://github.com/jekyll/jekyll
[jekyll-talk]: https://talk.jekyllrb.com/
