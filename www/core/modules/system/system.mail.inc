<?php
/**
 * @file
 * Backdrop core implementations of MailSystemInterface.
 */

/**
 * The default Backdrop mail backend using PHP's mail function.
 */
class DefaultMailSystem implements MailSystemInterface {
  /**
   * Concatenate and wrap the email body for plain-text mails.
   *
   * @param $message
   *   A message array, as described in hook_mail_alter().
   *
   * @return
   *   The formatted $message.
   */
  public function format(array $message) {
    // Join the body array into one string.
    $message['body'] = implode("\n\n", $message['body']);
    // Convert any HTML to plain-text and wrap the mail body for sending.
    // Note that backdrop_html_to_text() already calls backdrop_wrap_mail(), so
    // it is not necessary to call that explicitly again.
    $message['body'] = backdrop_html_to_text($message['body']);
    return $message;
  }

  /**
   * Send an email message, using Backdrop variables and default settings.
   *
   * @see http://php.net/manual/function.mail.php
   * @see backdrop_mail()
   *
   * @param $message
   *   A message array, as described in hook_mail_alter().
   * @return
   *   TRUE if the mail was successfully accepted, otherwise FALSE.
   */
  public function mail(array $message) {
    // If 'Return-Path' isn't already set in php.ini, we pass it separately
    // as an additional parameter instead of in the header.
    if (isset($message['headers']['Return-Path'])) {
      $return_path_set = strpos(ini_get('sendmail_path'), ' -f');
      if (!$return_path_set) {
        $message['Return-Path'] = $message['headers']['Return-Path'];
        unset($message['headers']['Return-Path']);
      }
    }
    $mimeheaders = array();
    foreach ($message['headers'] as $name => $value) {
      $mimeheaders[] = $name . ': ' . mime_header_encode($value);
    }
    $line_endings = settings_get('mail_line_endings', MAIL_LINE_ENDINGS);
    // Prepare mail commands.
    $mail_subject = mime_header_encode($message['subject']);
    // Note: email uses CRLF for line-endings. PHP's API requires LF
    // on Unix and CRLF on Windows. Backdrop automatically guesses the
    // line-ending format appropriate for your system. If you need to
    // override this, adjust $settings['mail_line_endings'] in settings.php.
    $mail_body = preg_replace('@\r?\n@', $line_endings, $message['body']);
    // For headers, PHP's API suggests that we use CRLF normally,
    // but some MTAs incorrectly replace LF with CRLF. See #234403.
    // $settings['mail_headers_line_endings'] will override this setting.
    $headers_line_endings = settings_get('mail_headers_line_endings', "\n");
    if (defined('PHP_VERSION_ID') && PHP_VERSION_ID >= 80000 ) {
      // PHP 8+ requires headers to be separated by CRLF, see:
      // - https://bugs.php.net/bug.php?id=81158
      // - https://github.com/php/php-src/commit/6983ae751cd301886c966b84367fc7aaa1273b2d#diff-c6922cd89f6f75912eb377833ca1eddb7dd41de088be821024b8a0e340fed3df
      $headers_line_endings = settings_get('mail_headers_line_endings', "\r\n");
    }
    $mail_headers = join($headers_line_endings, $mimeheaders);

    // We suppress warnings and notices from mail() because of issues on some
    // hosts. The return value of this method will still indicate whether mail
    // was sent successfully.
    if (!isset($_SERVER['WINDIR']) && strpos($_SERVER['SERVER_SOFTWARE'], 'Win32') === FALSE) {
      // We validate the return path, unless it is equal to the site mail, which
      // we assume to be safe.
      $additional_params = NULL;
      if (isset($message['Return-Path']) && (config_get('system.core', 'site_mail') === $message['Return-Path'] || self::_isShellSafe($message['Return-Path']))) {
        // On most non-Windows systems, the "-f" option to the sendmail command
        // is used to set the Return-Path. There is no space between -f and
        // the value of the return path.
        $additional_params = '-f' . $message['Return-Path'];
      }

      $mail_result = @mail(
        $message['to'],
        $mail_subject,
        $mail_body,
        $mail_headers,
        $additional_params
      );
    }
    else {
      // On Windows, PHP will use the value of sendmail_from for the
      // Return-Path header.
      $old_from = ini_get('sendmail_from');
      ini_set('sendmail_from', $message['Return-Path']);
      $mail_result = @mail(
        $message['to'],
        $mail_subject,
        $mail_body,
        $mail_headers
      );
      ini_set('sendmail_from', $old_from);
    }
    return $mail_result;
  }

  /**
   * Disallows potentially unsafe shell characters.
   *
   * Functionally similar to PHPMailer::isShellSafe() which resulted from
   * CVE-2016-10045. Note that escapeshellarg and escapeshellcmd are inadequate
   * for this purpose.
   *
   * @param string $string
   *   The string to be validated.
   *
   * @return bool
   *   True if the string is shell-safe.
   *
   * @see https://github.com/PHPMailer/PHPMailer/issues/924
   * @see https://github.com/PHPMailer/PHPMailer/blob/v5.2.21/class.phpmailer.php#L1430
   *
   * @todo Rename to ::isShellSafe() and/or discuss whether this is the correct
   *   location for this helper.
   */
  protected static function _isShellSafe($string) {
    if (escapeshellcmd($string) !== $string || !in_array(escapeshellarg($string), array("'$string'", "\"$string\""))) {
      return FALSE;
    }
    if (preg_match('/[^a-zA-Z0-9@_\-.]/', $string) !== 0) {
      return FALSE;
    }
    return TRUE;
  }
}

/**
 * A mail sending implementation that captures sent messages to a variable.
 *
 * This class is for running tests or for development.
 */
class TestingMailSystem extends DefaultMailSystem implements MailSystemInterface {
  /**
   * Accept an email message and store it in a variable.
   *
   * @param $message
   *   An email message.
   */
  public function mail(array $message) {
    $captured_emails = state_get('test_email_collector', array());
    $captured_emails[] = $message;
    state_set('test_email_collector', $captured_emails);
    return TRUE;
  }
}
