# Matomo AI Bot Tracking

Server-side tracking of AI assistants and chatbots (ChatGPT, Claude, Perplexity,
etc.) for Matomo Analytics.

AI assistants fetch pages over plain HTTP and never execute JavaScript, so the
Matomo module's client-side tracking code never fires for them. This module
detects those requests by their User-Agent and sends a server-to-server beacon
to Matomo's HTTP Tracking API instead, using the Bot Tracking parameters
introduced in Matomo 5.7.0.

- https://developer.matomo.org/api-reference/tracking-api#tracking-bots
- https://matomo.org/faq/how-to/set-up-ai-chatbot-tracking-with-the-http-tracking-api/

## Requirements

- The [Matomo](https://backdropcms.org/project/matomo) module, configured with a
  valid site ID and server URL. This module reads both from `matomo.settings`
  and does not duplicate them.
- Matomo 5.7.0 or later on the server side.

## Installation

- Install this module using the official Backdrop CMS instructions at
  https://backdropcms.org/guide/modules
- Configure the parent Matomo module first, at `admin/config/system/matomo`.
- Visit `admin/config/system/matomo/ai-tracking` and enable AI bot tracking.

## Configuration

At `admin/config/system/matomo/ai-tracking`:

- **Enable AI bot tracking** — master switch.
- **User-Agent patterns** — a case-insensitive regular expression fragment,
  alternatives separated by `|` or one per line. The form rejects invalid
  regular expressions and patterns that would also match regular browsers
  (for example an empty alternative such as a trailing `|`), since those would
  report every visitor as a chatbot. Leave empty to use the built-in default
  list, which mirrors
  the User-Agents Matomo currently classifies as live, user-triggered AI
  chatbots. Matomo decides chatbot classification on its own side: adding
  training or indexing crawlers (GPTBot, ClaudeBot, CCBot…) forwards those
  requests but they will not appear in the AI Chatbots reports.
- **token_auth** — optional, only needed if you want the visitor IP recorded.
  Matomo requires authentication for the `cip` parameter.

## Page cache

AI bots rarely carry a session cookie, so Backdrop's internal page cache serves
their requests directly and the request exits before `hook_init()` or
`hook_exit()` run. This module uses `hook_boot()`, the earliest hook available.

Backdrop does not invoke `hook_boot()` on page cache hits by default. Without
the setting below, tracking only fires on the first request after each cache
clear or expiry for a given URL, so a bot crawling fifty pages may show up as a
single hit. To cover cached pages, add this to `settings.php`:

```php
$settings['page_cache_invoke_hooks'] = TRUE;
```

### Side effects

This setting is not scoped to this module. It makes Backdrop invoke
`hook_boot()` **and** `hook_exit()` for every bootstrap module on the site, in a
phase where `common.inc` and most module APIs are not yet loaded. Modules that
implement those hooks assuming a full bootstrap will throw a fatal
`Call to undefined function` error, visible only to anonymous users.

Two known cases:

- **devel** — `devel_exit()` fails. Devel should not be enabled on a production
  site anyway.
- **gdpr_cookies** — `gdpr_cookies_exit()` fails. An issue is being filed
  upstream; the fix is a small guard at the top of the hook.

If you enable this setting, check which modules are involved with:

```sql
SELECT name, type FROM system WHERE bootstrap = 1 AND status = 1;
```

## Issues

Bugs and feature requests should be reported in the issue queue.

## Current Maintainers

- Robert Garrigos (https://github.com/robertgarrigos)

## License

This project is GPL v2 software. See the LICENSE.txt file in this directory for
complete text.
