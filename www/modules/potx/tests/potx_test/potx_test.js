
/**
 * Test Backdrop.t and Backdrop.formatPlural() invocation.
 */

Backdrop.t('Test string in JS');
Backdrop.formatPlural(count, '1 test string in JS', '@count test strings in JS');

Backdrop.t('');

Backdrop.t('String with @placeholder value', {'@placeholder': 'placeholder'});

Backdrop.t('Test string in JS in test context', {}, {'context': 'Test context'});
Backdrop.t('Test string in JS in context and with @placeholder', {'@placeholder': 'placeholder'}, {'context': 'Test context'});

Backdrop.t('Multiline string for the test with @placeholder',
  {'@placeholder': 'placeholder'},
  {'context': 'Test context'}
);

Backdrop.formatPlural(count, '1 test string in JS in test context', '@count test strings in JS in test context', {'@placeholder': 'placeholder', '%value': 12}, {'context': 'Test context'});
Backdrop.formatPlural(count, '1 test string in JS with context and @placeholder', '@count test strings in JS with context and @placeholder', {'@placeholder': 'placeholder', '%value': 12}, {'context': 'Test context'});
