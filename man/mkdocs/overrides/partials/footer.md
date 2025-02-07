{% if "navigation.footer" in features %} {% if page.previous_page or
page.next_page %} {% if page.meta and page.meta.hide %} {% set hidden =
"hidden" if "footer" in page.meta.hide %} {% endif %}

{% if page.previous_page %} {% set direction = lang.t("footer.previous")
%} <a href="%7B%7B-page.previous_page.url-%7C-url-%7D%7D"
class="md-footer__link md-footer__link--prev"
aria-label="{{ direction }}: {{ page.previous_page.title | e }}"></a>

{% set icon = config.theme.icon.previous or "material/arrow-left" %} {%
include ".icons/" ~ icon ~ ".svg" %}

<span class="md-footer__direction"> {{ direction }} </span>

{{ page.previous_page.title }}

{% endif %} {% if page.next_page %} {% set direction =
lang.t("footer.next") %}
<a href="%7B%7B-page.next_page.url-%7C-url-%7D%7D"
class="md-footer__link md-footer__link--next"
aria-label="{{ direction }}: {{ page.next_page.title | e }}"></a>

<span class="md-footer__direction"> {{ direction }} </span>

{{ page.next_page.title }}

{% set icon = config.theme.icon.next or "material/arrow-right" %} {%
include ".icons/" ~ icon ~ ".svg" %}

{% endif %}

{% endif %} {% endif %}

[Main index](./index.md) \| [Topics index](./topics.md) \| [Keywords
index](./keywords.md) \| [Graphical index](./graphical_index.md) \|
[Full index](./full_index.md)  

{% include "partials/copyright.html" %} {% if config.extra.social %} {%
include "partials/social.html" %} {% endif %}
