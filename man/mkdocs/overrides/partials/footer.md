{% if "navigation.footer" in features %} {% if page.previous_page or
page.next_page %} {% if page.meta and page.meta.hide %} {% set hidden =
"hidden" if "footer" in page.meta.hide %} {% endif %}

{% if page.previous_page %} {% set direction = lang.t("footer.previous")
%} <a href="%7B%7B-page.previous_page.url-%7C-url-%7D%7D"
class="md-footer__link md-footer__link--prev"
aria-label="{{ direction }}: {{ page.previous_page.title | e }}"></a>

<div class="md-footer__button md-icon">

{% set icon = config.theme.icon.previous or "material/arrow-left" %} {%
include ".icons/" ~ icon ~ ".svg" %}

</div>

<div class="md-footer__title">

<span class="md-footer__direction"> {{ direction }} </span>

<div class="md-ellipsis">

{{ page.previous_page.title }}

</div>

</div>

{% endif %} {% if page.next_page %} {% set direction =
lang.t("footer.next") %}
<a href="%7B%7B-page.next_page.url-%7C-url-%7D%7D"
class="md-footer__link md-footer__link--next"
aria-label="{{ direction }}: {{ page.next_page.title | e }}"></a>

<div class="md-footer__title">

<span class="md-footer__direction"> {{ direction }} </span>

<div class="md-ellipsis">

{{ page.next_page.title }}

</div>

</div>

<div class="md-footer__button md-icon">

{% set icon = config.theme.icon.next or "material/arrow-right" %} {%
include ".icons/" ~ icon ~ ".svg" %}

</div>

{% endif %}

{% endif %} {% endif %}

<div class="md-footer-meta md-typeset">

[Main index](./index.md) \| [Topics index](./topics.md) \| [Keywords
index](./keywords.md) \| [Graphical index](./graphical_index.md) \|
[Full index](./full_index.md)

<div class="md-footer-meta__inner md-grid">

{% include "partials/copyright.html" %} {% if config.extra.social %} {%
include "partials/social.html" %} {% endif %}

</div>

</div>
