<?php
/**
 * @file
 * Administrative page callbacks for the Matomo AI Bot Tracking module.
 */

/**
 * Implements hook_admin_settings() for configuring the module.
 */
function matomo_ai_tracking_admin_settings_form($form_state) {
  $config = config('matomo_ai_tracking.settings');
  $matomo_config = config('matomo.settings');

  if (!preg_match('/^\d{1,}$/', $matomo_config->get('site_id')) || empty($matomo_config->get('url_http'))) {
    backdrop_set_message(t('Configure the main <a href="@url">Matomo settings</a> (site ID and Matomo URL) first.', array('@url' => url('admin/config/system/matomo'))), 'warning');
  }

  $form['enabled'] = array(
    '#type' => 'checkbox',
    '#title' => t('Track AI assistants and crawlers server-side'),
    '#description' => t('AI assistants (ChatGPT, Perplexity, Claude, etc.) fetch pages without running JavaScript, so the Matomo tracking code never fires for them. If enabled, matching requests are reported to Matomo using its <a href="@url">Bot Tracking</a> parameters instead. Requires Matomo 5.7.0 or later.', array('@url' => 'https://developer.matomo.org/api-reference/tracking-api#tracking-bots')),
    '#default_value' => $config->get('enabled'),
  );

  $form['settings_php_notice'] = array(
    '#type' => 'item',
    '#markup' => '<div class="messages warning">' . t('<strong>Required extra step, outside this form:</strong> by default, Backdrop skips hook_boot() (which this feature relies on) for pages served from its internal page cache, so tracking would otherwise only fire on the first request after each cache clear/expiry. Add the following line to this site\'s <code>settings.php</code> &mdash; it cannot be set from within a module: <code>$settings[\'page_cache_invoke_hooks\'] = TRUE;</code>') . '</div>',
    '#states' => array(
      'visible' => array(
        ':input[name="enabled"]' => array('checked' => TRUE),
      ),
    ),
  );

  $form['patterns'] = array(
    '#type' => 'textarea',
    '#title' => t('User-Agent patterns (optional)'),
    '#default_value' => $config->get('patterns'),
    '#description' => t('User-Agent substrings/regular expression fragments, separated by the | character or one per line, that identify the AI assistants to track. Matching is case-insensitive. Leave empty to use the built-in default list shown below. The default value matches the User-Agents Matomo itself currently recognises as live, user-triggered AI chatbot requests (ChatGPT-User, Claude-User, Perplexity-User, MistralAI-User, Gemini-Deep-Research, GoogleAgent, NovaAct); Matomo determines chatbot classification independently on its side, so adding training/indexing crawlers here (GPTBot, ClaudeBot, PerplexityBot, CCBot, etc.) will not make them appear in the AI Chatbots reports, though the requests will still be forwarded.'),
    '#rows' => 4,
    '#states' => array(
      'visible' => array(
        ':input[name="enabled"]' => array('checked' => TRUE),
      ),
    ),
  );
  $form['patterns_example'] = array(
    '#type' => 'item',
    '#title' => t('Value to copy/paste'),
    '#markup' => '<code>' . check_plain(MATOMO_AI_TRACKING_PATTERNS) . '</code>',
    '#description' => t('The full list currently recognised by Matomo as live, user-triggered AI chatbot requests, ready to paste into the field above.'),
    '#states' => array(
      'visible' => array(
        ':input[name="enabled"]' => array('checked' => TRUE),
      ),
    ),
  );

  $form['token_auth'] = array(
    '#type' => 'textfield',
    '#title' => t('token_auth (optional)'),
    '#default_value' => $config->get('token_auth'),
    '#size' => 40,
    '#description' => t('Only needed if you also want the visitor IP address (cip) recorded with bot requests. Use a token belonging to a user with write permission on this website only, never the Super User token.'),
    '#states' => array(
      'visible' => array(
        ':input[name="enabled"]' => array('checked' => TRUE),
      ),
    ),
  );

  $form['#config'] = 'matomo_ai_tracking.settings';

  return system_settings_form($form);
}

/**
 * Form validation handler for matomo_ai_tracking_admin_settings_form().
 */
function matomo_ai_tracking_admin_settings_form_validate($form, &$form_state) {
  $form_state['values']['patterns'] = trim($form_state['values']['patterns']);
  $form_state['values']['token_auth'] = trim($form_state['values']['token_auth']);

  // Validate the patterns exactly as matomo_ai_tracking_boot() will use them.
  $regex = matomo_ai_tracking_patterns_regex($form_state['values']['patterns']);
  if (@preg_match($regex, '') === FALSE) {
    form_set_error('patterns', t('The User-Agent patterns are not a valid regular expression. Check for unbalanced brackets or unescaped special characters such as ( ) [ ] { } + * ?'));
    return;
  }

  // An empty alternative (e.g. a leading, trailing or doubled "|") or a
  // pattern like ".*" matches every request, which would report every
  // visitor to Matomo as an AI chatbot.
  $browsers = array(
    '',
    'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/130.0.0.0 Safari/537.36',
    'Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:132.0) Gecko/20100101 Firefox/132.0',
    'Mozilla/5.0 (iPhone; CPU iPhone OS 18_0 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/18.0 Mobile/15E148 Safari/604.1',
  );
  foreach ($browsers as $browser) {
    if (preg_match($regex, $browser)) {
      form_set_error('patterns', t('The User-Agent patterns also match regular browsers, so every visitor would be reported as an AI chatbot. Check for empty alternatives (a leading, trailing or doubled | character) or overly broad patterns.'));
      return;
    }
  }

  if ($form_state['values']['token_auth'] !== '' && !preg_match('/^[a-zA-Z0-9]{32}$/', $form_state['values']['token_auth'])) {
    form_set_error('token_auth', t('The token_auth must be a 32-character alphanumeric string.'));
  }
}
