<!doctype html>
<html xmlns="http://www.w3.org/1999/xhtml" lang="$lang$" xml:lang="$lang$" $if(dir)$ dir="$dir$" $endif$>
  <head>
    <meta charset="utf-8" />
    <meta
      name="viewport"
      content="width=device-width, initial-scale=1.0, user-scalable=yes"
    />
    <meta name="author" content="$author-meta$" />
    <meta name="dcterms.date" content="$date-meta$" />
    <meta name="keywords" content="$for(keywords)$$keywords$$sep$, $endfor$" />
    $if(description-meta)$
    <meta name="description" content="$description-meta$" />
    $endif$
    <title>$title$</title>
    <style>
      $styles.html()$
    </style>
    <link rel="stylesheet" href="$css$" />
    $header-includes$
  </head>
  <body>
    $include-before$
    $if(title)$
    <header id="title-block-header">
    <h1 class="title">$title$</h1>
    </header>
    $endif$
    <ul>
      $for(pages)$
      <li>
        <a class="index-el" href="$it.path$"><span class="title">$it.title$</span></a> <span class="date-index">$it.date$</span> 
      </li>
      $endfor$
    </ul>
    $include-after$
  </body>
</html>
