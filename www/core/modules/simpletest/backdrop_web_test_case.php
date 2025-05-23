<?php
/**
 * Global variable that holds information about the tests being run.
 *
 * An array, with the following keys:
 *  - 'test_run_id': the ID of the test being run, in the form 'simpletest_%"
 *  - 'in_child_site': TRUE if the current request is a cURL request from
 *     the parent site.
 *
 * @var array
 */
global $backdrop_test_info;

/**
 * Base class for Backdrop tests.
 *
 * Do not extend this class, use one of the subclasses in this file.
 */
abstract class BackdropTestCase {
  /**
   * The test run ID.
   *
   * @var string
   */
  protected $testId;

  /**
   * The database prefix of this test run.
   *
   * @var string
   */
  protected $databasePrefix = NULL;

  /**
   * The file directory for this test's files. Usually matches $databasePrefix.
   *
   * @var string
   */
  protected $fileDirectoryName = NULL;

  /**
   * The original file directory, before it was changed for testing purposes.
   *
   * @var string
   */
  protected $originalFileDirectory = NULL;

  /**
   * The original config directory array, before changing for testing purposes.
   *
   * @var string
   */
  protected $originalConfigDirectories = NULL;

  /**
   * URL to the verbose output file directory.
   *
   * @var string
   */
  protected $verboseDirectoryUrl;

  /**
   * Time limit for the test.
   */
  protected $timeLimit = 500;

  /**
   * Current results of this test case.
   *
   * @var array
   */
  public $results = array(
    '#pass' => 0,
    '#fail' => 0,
    '#exception' => 0,
    '#debug' => 0,
    '#duration' => 0,
  );

  /**
   * Assertions thrown in that test case.
   *
   * @var array
   */
  protected $assertions = array();

  /**
   * This class is skipped when looking for the source of an assertion.
   *
   * When displaying which function an assert comes from, it's not too useful
   * to see "backdropWebTestCase->backdropLogin()', we would like to see the test
   * that called it. So we need to skip the classes defining these helper
   * methods.
   */
  protected $skipClasses = array(__CLASS__ => TRUE);

  /**
   * Flag to indicate whether the test has been set up.
   *
   * The setUp() method isolates the test from the parent Backdrop site by
   * creating a random prefix for the database and setting up a clean file
   * storage directory. The tearDown() method then cleans up this test
   * environment. We must ensure that setUp() has been run. Otherwise,
   * tearDown() will act on the parent Backdrop site rather than the test
   * environment, destroying live data.
   */
  protected $setup = FALSE;

  protected $setupDatabasePrefix = FALSE;

  protected $setupEnvironment = FALSE;

  /**
   * HTTP authentication method
   */
  protected $httpauth_method = CURLAUTH_BASIC;

  /**
   * HTTP authentication credentials (<username>:<password>).
   */
  protected $httpauth_credentials = NULL;

  /**
   * Constructor for BackdropTestCase.
   *
   * @param $test_id
   *   Tests with the same id are reported together.
   */
  public function __construct($test_id = NULL) {
    $this->testId = $test_id;
  }

  /**
   * Checks the matching requirements for BackdropTestCase.
   *
   * @return
   *   Array of errors containing a list of unmet requirements.
   */
  protected function checkRequirements() {
    $errors = array();

    if (!extension_loaded('curl')) {
      $errors[] = 'PHP curl extension is not present.';
    }

    return $errors;
  }

  /**
   * Generates a database prefix for running tests.
   *
   * The generated database table prefix is used for the Backdrop installation
   * being performed for the test. It is also used as user agent HTTP header
   * value by the cURL-based browser of BackdropWebTestCase, which is sent
   * to the Backdrop installation of the test. During early Backdrop bootstrap,
   * the user agent HTTP header is parsed, and if it matches, all database
   * queries use the database table prefix that has been generated here.
   *
   * @see BackdropWebTestCase::curlInitialize()
   * @see backdrop_valid_test_ua()
   * @see BackdropWebTestCase::setUp()
   */
  protected function prepareDatabasePrefix() {
    // Generate a temporary prefixed database to ensure that tests have a clean
    // starting point and confirm that random prefix isn't already in use.
    db_transaction();
    do {
      $prefix = 'simpletest' . mt_rand(100000, 999999);
      $prefix_exists = db_query("SELECT COUNT(*) FROM {simpletest_prefix} WHERE prefix = :prefix", array(':prefix' => $prefix))->fetchField();
    } while ($prefix_exists);
    $this->databasePrefix = $prefix;
    $this->fileDirectoryName = substr($prefix, 10);

    // As soon as the database prefix is set, the test might start to execute.
    // All assertions as well as the SimpleTest batch operations are associated
    // with the testId, so the database prefix has to be associated with it.
    db_insert('simpletest_prefix')
      ->fields(array(
        'test_id' => $this->testId,
        'prefix' => $this->databasePrefix,
      ))
      ->execute();
  }

  /**
   * Internal helper: stores the assert.
   *
   * @param $status
   *   Can be 'pass', 'fail', 'exception'.
   *   TRUE is a synonym for 'pass', FALSE for 'fail'.
   * @param $message
   *   The message string.
   * @param $group
   *   Which group this assert belongs to.
   * @param $caller
   *   By default, the assert comes from a function whose name starts with
   *   'test'. Instead, you can specify where this assert originates from
   *   by passing in an associative array as $caller. Key 'file' is
   *   the name of the source file, 'line' is the line number and 'function'
   *   is the caller function itself.
   *
   * @return bool
   *   TRUE if the assertion passed and FALSE if it failed.
   */
  protected function assert($status, $message = '', $group = 'Other', array $caller = array()) {
    // Convert boolean status to string status.
    if (is_bool($status)) {
      $status = $status ? 'pass' : 'fail';
    }

    // Increment summary result counter.
    $this->results['#' . $status]++;

    // Get the function information about the call to the assertion method.
    if (!$caller) {
      $caller = $this->getAssertionCall();
    }

    // Creation assertion array that can be displayed while tests are running.
    $this->assertions[] = $assertion = array(
      'test_id' => $this->testId,
      'test_class' => get_class($this),
      'status' => $status,
      'message' => $message,
      'message_group' => $group,
      'function' => $caller['function'],
      'line' => $caller['line'],
      'file' => $caller['file'],
    );

    // Store assertion for display after the test has completed.
    self::getDatabaseConnection()
      ->insert('simpletest')
      ->fields($assertion)
      ->execute();

    // We do not use a ternary operator here to allow a breakpoint on
    // test failure.
    if ($status == 'pass') {
      return TRUE;
    }
    else {
      return FALSE;
    }
  }

  /**
   * Returns the database connection to the site running Simpletest.
   *
   * @return DatabaseConnection
   *   The database connection to use for inserting assertions.
   */
  public static function getDatabaseConnection() {
    try {
      $connection = Database::getConnection('default', 'simpletest_original_default');
    }
    catch (DatabaseConnectionNotDefinedException $e) {
      // If the test was not set up, the simpletest_original_default
      // connection does not exist.
      $connection = Database::getConnection('default', 'default');
    }

    return $connection;
  }

  /**
   * Store an assertion from outside the testing context.
   *
   * This is useful for inserting assertions that can only be recorded after
   * the test case has been destroyed, such as PHP fatal errors. The caller
   * information is not automatically gathered since the caller is most likely
   * inserting the assertion on behalf of other code. In all other respects
   * the method behaves just like BackdropTestCase::assert() in terms of storing
   * the assertion.
   *
   * @return
   *   Message ID of the stored assertion.
   *
   * @see BackdropTestCase::assert()
   * @see BackdropTestCase::deleteAssert()
   */
  public static function insertAssert($test_id, $test_class, $status, $message = '', $group = 'Other', array $caller = array()) {
    // Convert boolean status to string status.
    if (is_bool($status)) {
      $status = $status ? 'pass' : 'fail';
    }

    $caller += array(
      'function' => t('Unknown'),
      'line' => 0,
      'file' => t('Unknown'),
    );

    $assertion = array(
      'test_id' => $test_id,
      'test_class' => $test_class,
      'status' => $status,
      'message' => $message,
      'message_group' => $group,
      'function' => $caller['function'],
      'line' => $caller['line'],
      'file' => $caller['file'],
    );

    return self::getDatabaseConnection()
      ->insert('simpletest')
      ->fields($assertion)
      ->execute();
  }

  /**
   * Delete an assertion record by message ID.
   *
   * @param $message_id
   *   Message ID of the assertion to delete.
   * @return
   *   TRUE if the assertion was deleted, FALSE otherwise.
   *
   * @see BackdropTestCase::insertAssert()
   */
  public static function deleteAssert($message_id) {
    return (bool) self::getDatabaseConnection()
      ->delete('simpletest')
      ->condition('message_id', $message_id)
      ->execute();
  }

  /**
   * Cycles through backtrace until the first non-assertion method is found.
   *
   * @return
   *   Array representing the true caller.
   */
  protected function getAssertionCall() {
    $backtrace = debug_backtrace();

    // The first element is the call. The second element is the caller.
    // We skip calls that occurred in one of the methods of our base classes
    // or in an assertion function.
   while (($caller = $backtrace[1]) &&
         ((isset($caller['class']) && isset($this->skipClasses[$caller['class']])) ||
           substr($caller['function'], 0, 6) == 'assert')) {
      // We remove that call.
      array_shift($backtrace);
    }

    return _backdrop_get_last_caller($backtrace);
  }

  /**
   * Check to see if a value is not false (not an empty string, 0, NULL, or FALSE).
   *
   * @param $value
   *   The value on which the assertion is to be done.
   * @param $message
   *   The message to display along with the assertion.
   * @param $group
   *   The type of assertion - examples are "Browser", "PHP".
   * @return boolean
   *   TRUE if the assertion succeeded, FALSE otherwise.
   */
  protected function assertTrue($value, $message = '', $group = 'Other') {
    return $this->assert((bool) $value, $message ? $message : t('Value @value is TRUE.', array('@value' => var_export($value, TRUE))), $group);
  }

  /**
   * Check to see if a value is false (an empty string, 0, NULL, or FALSE).
   *
   * @param $value
   *   The value on which the assertion is to be done.
   * @param $message
   *   The message to display along with the assertion.
   * @param $group
   *   The type of assertion - examples are "Browser", "PHP".
   * @return
   *   TRUE if the assertion succeeded, FALSE otherwise.
   */
  protected function assertFalse($value, $message = '', $group = 'Other') {
    return $this->assert(!$value, $message ? $message : t('Value @value is FALSE.', array('@value' => var_export($value, TRUE))), $group);
  }

  /**
   * Check to see if a value is NULL.
   *
   * @param $value
   *   The value on which the assertion is to be done.
   * @param $message
   *   The message to display along with the assertion.
   * @param $group
   *   The type of assertion - examples are "Browser", "PHP".
   * @return
   *   TRUE if the assertion succeeded, FALSE otherwise.
   */
  protected function assertNull($value, $message = '', $group = 'Other') {
    return $this->assert(!isset($value), $message ? $message : t('Value @value is NULL.', array('@value' => var_export($value, TRUE))), $group);
  }

  /**
   * Check to see if a value is not NULL.
   *
   * @param $value
   *   The value on which the assertion is to be done.
   * @param $message
   *   The message to display along with the assertion.
   * @param $group
   *   The type of assertion - examples are "Browser", "PHP".
   * @return
   *   TRUE if the assertion succeeded, FALSE otherwise.
   */
  protected function assertNotNull($value, $message = '', $group = 'Other') {
    return $this->assert(isset($value), $message ? $message : t('Value @value is not NULL.', array('@value' => var_export($value, TRUE))), $group);
  }

  /**
   * Check to see if two values are equal.
   *
   * @param $first
   *   The first value to check.
   * @param $second
   *   The second value to check.
   * @param $message
   *   The message to display along with the assertion.
   * @param $group
   *   The type of assertion - examples are "Browser", "PHP".
   * @return
   *   TRUE if the assertion succeeded, FALSE otherwise.
   */
  protected function assertEqual($first, $second, $message = '', $group = 'Other') {
    return $this->assert($first == $second, $message ? $message : t('Value @first is equal to value @second.', array('@first' => var_export($first, TRUE), '@second' => var_export($second, TRUE))), $group);
  }

  /**
   * Check to see if two values are not equal.
   *
   * @param $first
   *   The first value to check.
   * @param $second
   *   The second value to check.
   * @param $message
   *   The message to display along with the assertion.
   * @param $group
   *   The type of assertion - examples are "Browser", "PHP".
   * @return
   *   TRUE if the assertion succeeded, FALSE otherwise.
   */
  protected function assertNotEqual($first, $second, $message = '', $group = 'Other') {
    return $this->assert($first != $second, $message ? $message : t('Value @first is not equal to value @second.', array('@first' => var_export($first, TRUE), '@second' => var_export($second, TRUE))), $group);
  }

  /**
   * Check to see if two values are identical.
   *
   * @param $first
   *   The first value to check.
   * @param $second
   *   The second value to check.
   * @param $message
   *   The message to display along with the assertion.
   * @param $group
   *   The type of assertion - examples are "Browser", "PHP".
   * @return
   *   TRUE if the assertion succeeded, FALSE otherwise.
   */
  protected function assertIdentical($first, $second, $message = '', $group = 'Other') {
    return $this->assert($first === $second, $message ? $message : t('Value @first is identical to value @second.', array('@first' => var_export($first, TRUE), '@second' => var_export($second, TRUE))), $group);
  }

  /**
   * Check to see if two values are not identical.
   *
   * @param $first
   *   The first value to check.
   * @param $second
   *   The second value to check.
   * @param $message
   *   The message to display along with the assertion.
   * @param $group
   *   The type of assertion - examples are "Browser", "PHP".
   * @return
   *   TRUE if the assertion succeeded, FALSE otherwise.
   */
  protected function assertNotIdentical($first, $second, $message = '', $group = 'Other') {
    return $this->assert($first !== $second, $message ? $message : t('Value @first is not identical to value @second.', array('@first' => var_export($first, TRUE), '@second' => var_export($second, TRUE))), $group);
  }

  /**
   * Fire an assertion that is always positive.
   *
   * @param $message
   *   The message to display along with the assertion.
   * @param $group
   *   The type of assertion - examples are "Browser", "PHP".
   * @return
   *   TRUE.
   */
  protected function pass($message = NULL, $group = 'Other') {
    return $this->assert(TRUE, $message, $group);
  }

  /**
   * Fire an assertion that is always negative.
   *
   * @param $message
   *   The message to display along with the assertion.
   * @param $group
   *   The type of assertion - examples are "Browser", "PHP".
   * @return
   *   FALSE.
   */
  protected function fail($message = NULL, $group = 'Other') {
    return $this->assert(FALSE, $message, $group);
  }

  /**
   * Fire an error assertion.
   *
   * @param $message
   *   The message to display along with the assertion.
   * @param $group
   *   The type of assertion - examples are "Browser", "PHP".
   * @param $caller
   *   The caller of the error.
   * @return false
   *   Always returns false.
   */
  protected function error($message = '', $group = 'Other', array $caller = array()) {
    if ($group == 'User notice') {
      // Since 'User notice' is set by trigger_error() which is used for debug
      // set the message to a status of 'debug'.
      return $this->assert('debug', $message, 'Debug', $caller);
    }

    return $this->assert('exception', $message, $group, $caller);
  }

  /**
   * Logs a verbose message in a text file.
   *
   * The link to the verbose message will be placed in the test results as a
   * passing assertion with the text '[verbose message]'.
   *
   * @param $message
   *   The verbose message to be stored.
   *
   * @see simpletest_verbose()
   */
  protected function verbose($message) {
    if ($id = simpletest_verbose($message)) {
      $class_safe = str_replace('\\', '_', get_class($this));
      $url = $this->verboseDirectoryUrl . '/' . $class_safe . '-' . $id . '.html';
      // Not using l() to avoid invoking the theme system, so that unit tests
      // can use verbose() as well.
      $link = '<a href="' . $url . '" target="_blank">' . t('Verbose message') . '</a>';
      $this->error($link, 'User notice');
    }
  }

  /**
   * Run all tests in this class.
   *
   * Regardless of whether $methods are passed or not, only method names
   * starting with "test" are executed.
   *
   * @param $methods
   *   (optional) A list of method names in the test case class to run; e.g.,
   *   array('testFoo', 'testBar'). By default, all methods of the class are
   *   taken into account, but it can be useful to only run a few selected test
   *   methods during debugging.
   */
  public function run(array $methods = array()) {
    // Get start time so that we can see how long tests are taking.
    $start = microtime(TRUE);

    $config = config('simpletest.settings');
    // Initialize verbose debugging.
    simpletest_verbose(NULL, config_get('system.core', 'file_public_path'), get_class($this));

    // HTTP auth settings (<username>:<password>) for the simpletest browser
    // when sending requests to the test site.
    $this->httpauth_method = $config->get('simpletest_method');
    $username = $config->get('simpletest_username');
    $password = $config->get('simpletest_password');
    if (!empty($username) && !empty($password)) {
      $this->httpauth_credentials = $username . ':' . $password;
    }

    set_error_handler(array($this, 'errorHandler'));
    $class = get_class($this);
    // Iterate through all the methods in this class, unless a specific list of
    // methods to run was passed.
    $class_methods = get_class_methods($class);
    if ($methods) {
      $class_methods = array_intersect($class_methods, $methods);
    }
    $missing_requirements = $this->checkRequirements();
    if (!empty($missing_requirements)) {
      foreach ($missing_requirements as $missing_requirement) {
        $this->fail($missing_requirement, 'Requirements check.');
      }
    }
    else {
      foreach ($class_methods as $method) {
        // If the current method starts with "test", run it - it's a test.
        if (strtolower(substr($method, 0, 4)) == 'test') {
          // Insert a fail record. This will be deleted on completion to ensure
          // that testing completed.
          $method_info = new ReflectionMethod($class, $method);
          $caller = array(
            'file' => $method_info->getFileName(),
            'line' => $method_info->getStartLine(),
            'function' => $class . '->' . $method . '()',
          );
          $completion_check_id = BackdropTestCase::insertAssert($this->testId, $class, FALSE, t('The test did not complete due to a fatal error.'), 'Completion check', $caller);
          $this->setUp();
          if ($this->setup) {
            try {
              $this->$method();
              // Finish up.
            }
            catch (Exception $e) {
              $this->exceptionHandler($e);
            }
            $this->tearDown();
          }
          else {
            $this->fail(t("The test cannot be executed because it has not been set up properly."));
          }
          // Remove the completion check record.
          BackdropTestCase::deleteAssert($completion_check_id);
        }
      }
    }
    // Clear out the error messages and restore error handler.
    backdrop_get_messages();
    restore_error_handler();

    // Get the stop time and put it in the results to display later.
    $end = microtime(TRUE);
    $this->results['#duration'] = round($end - $start, 3);
  }

  /**
   * Prepare an environment in which tests will be executed.
   *
   * This may be use used to create databases, install Backdrop and dependent
   * modules, or do any other work necessary to prepare a testing environment
   * for each test that will be run.
   */
  protected abstract function setUp();

  /**
   * Clean up any database tables, configuration, or files after each test run.
   */
  protected abstract function tearDown();

  /**
   * Handle errors during test runs.
   *
   * Because this is registered in set_error_handler(), it has to be public.
   * @see set_error_handler
   */
  public function errorHandler($severity, $message, $file = NULL, $line = NULL) {
    if ($severity & error_reporting()) {
      $error_map = array(
        E_WARNING => 'Warning',
        E_NOTICE => 'Notice',
        E_CORE_ERROR => 'Core error',
        E_CORE_WARNING => 'Core warning',
        E_USER_ERROR => 'User error',
        E_USER_WARNING => 'User warning',
        E_USER_NOTICE => 'User notice',
        E_RECOVERABLE_ERROR => 'Recoverable error',
        E_DEPRECATED => 'Deprecated',
        E_USER_DEPRECATED => 'User deprecated',
      );
      // E_STRICT was removed from PHP 8.4 and higher, but still exists in older
      // versions.
      if (version_compare(PHP_VERSION, '8.4.0') < 0) {
        $error_map[E_STRICT] = 'Run-time notice';
      }

      $backtrace = debug_backtrace();
      $this->error($message, $error_map[$severity], _backdrop_get_last_caller($backtrace));
    }
    return TRUE;
  }

  /**
   * Handle exceptions.
   *
   * @see set_exception_handler
   */
  protected function exceptionHandler(Exception $exception) {
    $backtrace = $exception->getTrace();
    // Push on top of the backtrace the call that generated the exception.
    array_unshift($backtrace, array(
      'line' => $exception->getLine(),
      'file' => $exception->getFile(),
    ));
    require_once BACKDROP_ROOT . '/core/includes/errors.inc';
    // The exception message is run through check_plain() by _backdrop_decode_exception().
    $this->error(t('%type: !message in %function (line %line of %file).', _backdrop_decode_exception($exception)), 'Uncaught exception', _backdrop_get_last_caller($backtrace));
  }

  /**
   * Generates a random string of ASCII characters of codes 32 to 126.
   *
   * The generated string includes alpha-numeric characters and common
   * miscellaneous characters. Use this method when testing general input
   * where the content is not restricted.
   *
   * Do not use this method when special characters are not possible (e.g., in
   * machine or file names that have already been validated); instead,
   * use BackdropWebTestCase::randomName().
   *
   * @param $length
   *   Length of random string to generate.
   *
   * @return
   *   Randomly generated string.
   *
   * @see BackdropWebTestCase::randomName()
   */
  public static function randomString($length = 8) {
    $str = '';
    for ($i = 0; $i < $length; $i++) {
      $str .= chr(mt_rand(32, 126));
    }
    return $str;
  }

  /**
   * Generates a random string containing letters and numbers.
   *
   * The string will always start with a letter. The letters may be upper or
   * lower case. This method is better for restricted inputs that do not
   * accept certain characters. For example, when testing input fields that
   * require machine readable values (i.e. without spaces and non-standard
   * characters) this method is best.
   *
   * Do not use this method when testing unvalidated user input. Instead, use
   * BackdropWebTestCase::randomString().
   *
   * @param $length
   *   Length of random string to generate.
   *
   * @return
   *   Randomly generated string.
   *
   * @see BackdropWebTestCase::randomString()
   */
  public static function randomName($length = 8) {
    $values = array_merge(range(65, 90), range(97, 122), range(48, 57));
    $max = count($values) - 1;
    $str = chr(mt_rand(97, 122));
    for ($i = 1; $i < $length; $i++) {
      $str .= chr($values[mt_rand(0, $max)]);
    }
    return $str;
  }

  /**
   * Converts a list of possible parameters into a stack of permutations.
   *
   * Takes a list of parameters containing possible values, and converts all of
   * them into a list of items containing every possible permutation.
   *
   * Example:
   * @code
   * $parameters = array(
   *   'one' => array(0, 1),
   *   'two' => array(2, 3),
   * );
   * $permutations = BackdropTestCase::generatePermutations($parameters);
   * // Result:
   * $permutations == array(
   *   array('one' => 0, 'two' => 2),
   *   array('one' => 1, 'two' => 2),
   *   array('one' => 0, 'two' => 3),
   *   array('one' => 1, 'two' => 3),
   * )
   * @endcode
   *
   * @param $parameters
   *   An associative array of parameters, keyed by parameter name, and whose
   *   values are arrays of parameter values.
   *
   * @return
   *   A list of permutations, which is an array of arrays. Each inner array
   *   contains the full list of parameters that have been passed, but with a
   *   single value only.
   */
  public static function generatePermutations($parameters) {
    $all_permutations = array(array());
    foreach ($parameters as $parameter => $values) {
      $new_permutations = array();
      // Iterate over all values of the parameter.
      foreach ($values as $value) {
        // Iterate over all existing permutations.
        foreach ($all_permutations as $permutation) {
          // Add the new parameter value to existing permutations.
          $new_permutations[] = $permutation + array($parameter => $value);
        }
      }
      // Replace the old permutations with the new permutations.
      $all_permutations = $new_permutations;
    }
    return $all_permutations;
  }
}

/**
 * Test case for Backdrop unit tests.
 *
 * These tests can not access the database nor files. Calling any Backdrop
 * function that needs the database will throw exceptions. These include
 * watchdog(), module_implements(), module_invoke_all() etc.
 */
class BackdropUnitTestCase extends BackdropTestCase {
  /**
   * The list of enabled modules prior to changing for testing.
   *
   * @var array
   */
  protected $originalModuleList;

  /**
   * @var object
   */
  protected $originalLanguage;

  /**
   * Constructor for BackdropUnitTestCase.
   */
  function __construct($test_id = NULL) {
    parent::__construct($test_id);
    $this->skipClasses[__CLASS__] = TRUE;
  }

  /**
   * Sets up unit test environment.
   *
   * Unlike BackdropWebTestCase::setUp(), BackdropUnitTestCase::setUp() does not
   * install modules because tests are performed without accessing the database.
   * Any required files must be explicitly included by the child class setUp()
   * method.
   */
  protected function setUp() {
    global $conf, $language;

    // Store necessary current values before switching to the test environment.
    $this->originalFileDirectory = config_get('system.core', 'file_public_path');
    $this->verboseDirectoryUrl = file_create_url($this->originalFileDirectory . '/simpletest/verbose');

    // Set to English to prevent any use of the database in t() calls.
    // The following array/object conversion is copied from language_default().
    $this->originalLanguage = $language;
    $language = (object) array(
      'langcode' => 'en',
      'name' => 'English',
      'direction' => 0,
      'enabled' => TRUE,
      'weight' => 0,
    );

    $this->prepareDatabasePrefix();

    // Reset all statics so that test is performed with a clean environment.
    backdrop_static_reset();

    // Create test directory.
    $public_files_directory = $this->originalFileDirectory . '/simpletest/' . $this->fileDirectoryName;
    file_prepare_directory($public_files_directory, FILE_CREATE_DIRECTORY | FILE_MODIFY_PERMISSIONS);
    $conf['file_public_path'] = $public_files_directory;

    // Clone the current connection and replace the current prefix.
    $connection_info = Database::getConnectionInfo('default');
    Database::renameConnection('default', 'simpletest_original_default');
    foreach ($connection_info as $target => $value) {
      $connection_info[$target]['prefix'] = array(
        'default' => $value['prefix']['default'] . $this->databasePrefix,
      );
    }
    Database::addConnectionInfo('default', 'default', $connection_info['default']);

    // Set user agent to be consistent with web test case.
    $_SERVER['HTTP_USER_AGENT'] = $this->databasePrefix;

    // If locale is enabled then t() will try to access the database and
    // subsequently will fail as the database is not accessible.
    $module_list = module_list();
    if (isset($module_list['locale'])) {
      // Transform the list into the format expected as input to module_list().
      foreach ($module_list as &$module) {
        $module = array('filename' => backdrop_get_filename('module', $module));
      }
      $this->originalModuleList = $module_list;
      unset($module_list['locale']);
      module_list(TRUE, FALSE, FALSE, $module_list);
    }
    $this->setup = TRUE;
    return TRUE;
  }

  protected function tearDown() {
    global $conf, $language;

    // Get back to the original connection.
    Database::removeConnection('default');
    Database::renameConnection('simpletest_original_default', 'default');

    // Delete the database table prefix record.
    db_delete('simpletest_prefix')
      ->condition('test_id', $this->testId)
      ->condition('prefix', $this->databasePrefix)
      ->execute();

    $conf['file_public_path'] = $this->originalFileDirectory;
    // Restore modules if necessary.
    if (isset($this->originalModuleList)) {
      module_list(TRUE, FALSE, FALSE, $this->originalModuleList);
    }

    // Reset language.
    $language = $this->originalLanguage;
  }
}

/**
 * Test case for typical Backdrop tests.
 */
class BackdropWebTestCase extends BackdropTestCase {
  /**
   * The profile to install as a basis for testing.
   *
   * @var string
   */
  protected $profile = 'standard';

  /**
   * The URL currently loaded in the internal browser.
   *
   * @var string
   */
  protected $url;

  /**
   * The handle of the current cURL connection.
   *
   * @var resource
   */
  protected $curlHandle;

  /**
   * The headers of the page currently loaded in the internal browser.
   *
   * @var array
   */
  protected $headers;

  /**
   * The content of the page currently loaded in the internal browser.
   *
   * @var string|false
   */
  protected $content;

  /**
   * The content of the page currently loaded in the internal browser (plain text version).
   *
   * @var string|false
   */
  protected $plainTextContent;

  /**
   * The value of the Backdrop.settings JavaScript variable for the page currently loaded in the internal browser.
   *
   * @var array
   */
  protected $backdropSettings;

  /**
   * The parsed version of the page.
   *
   * @var SimpleXMLElement
   */
  protected $elements = NULL;

  /**
   * The current user logged in using the internal browser.
   *
   * @var FALSE|User
   */
  protected $loggedInUser = FALSE;

  /**
   * The current cookie file used by cURL.
   *
   * We do not reuse the cookies in further runs, so we do not need a file
   * but we still need cookie handling, so we set the jar to NULL.
   */
  protected $cookieFile = NULL;

  /**
   * The cookies of the page currently loaded in the internal browser.
   *
   * @var array
   */
  protected $cookies = array();

  /**
   * Additional cURL options.
   *
   * BackdropWebTestCase itself never sets this but always obeys what is set.
   */
  protected $additionalCurlOptions = array();

  /**
   * The original user, before it was changed to a clean uid = 1 for testing purposes.
   *
   * @var object
   */
  protected $originalUser = NULL;

  /**
   * The original settings as provided in settings.php.
   *
   * @var object
   */
  protected $originalSettings = NULL;

  /**
   * The original installation profile, before changing for testing purposes.
   *
   * @var string
   */
  protected $originalProfile;

  /**
   * The original clean URL setting, before changing for testing purposes.
   * @var bool
   */
  protected $originalCleanUrl;

  /**
   * The original shutdown handlers array, before it was cleaned for testing.
   *
   * @var array
   */
  protected $originalShutdownCallbacks = array();

  /**
   * The current session name, if available.
   */
  protected $session_name = NULL;

  /**
   * The current session ID, if available.
   */
  protected $session_id = NULL;

  /**
   * Whether the files were copied to the test files directory.
   */
  protected $generatedTestFiles = array();

  /**
   * The maximum number of redirects to follow when handling responses.
   */
  protected $maximumRedirects = 5;

  /**
   * The number of redirects followed during the handling of a request.
   */
  protected $redirect_count;

  public $public_files_directory;
  public $private_files_directory;
  public $temp_files_directory;

  /**
   * @var object
   */
  protected $originalLanguage;

  /**
   * @var object
   */
  protected $originalLanguageUrl;

  /**
   * Constructor for BackdropWebTestCase.
   */
  function __construct($test_id = NULL) {
    parent::__construct($test_id);
    $this->skipClasses[__CLASS__] = TRUE;
  }

  /**
   * Get a node from the database based on its title.
   *
   * @param $title
   *   A node title, usually generated by $this->randomName().
   * @param $reset
   *   (optional) Whether to reset the internal node_load() cache.
   *
   * @return
   *   A node entity matching $title.
   */
  function backdropGetNodeByTitle($title, $reset = FALSE) {
    $nodes = node_load_multiple(array(), array('title' => $title), $reset);
    // Load the first node returned from the database.
    $returned_node = reset($nodes);
    return $returned_node;
  }

  /**
   * Creates a node based on default settings.
   *
   * @param $settings
   *   An associative array of settings to change from the defaults, keys are
   *   node properties, for example 'title' => 'Hello, world!'.
   * @return Node
   *   Created node entity.
   */
  protected function backdropCreateNode($settings = array()) {
    // Populate defaults array.
    $settings += array(
      'body'      => array(LANGUAGE_NONE => array(array())),
      'title'     => $this->randomName(8),
      'comment'   => 2,
      'changed'   => REQUEST_TIME,
      'moderate'  => 0,
      'promote'   => 0,
      'revision'  => 1,
      'log'       => '',
      'status'    => 1,
      'sticky'    => 0,
      'type'      => 'page',
      'revisions' => NULL,
      'langcode'  => LANGUAGE_NONE,
    );

    // Use the original node's created time for existing nodes.
    if (isset($settings['created']) && !isset($settings['date'])) {
      $settings['date'] = format_date($settings['created'], 'custom', 'Y-m-d H:i:s O');
    }

    // If the node's user uid is not specified manually, use the currently
    // logged in user if available, or else the user running the test.
    if (!isset($settings['uid'])) {
      if ($this->loggedInUser) {
        $settings['uid'] = $this->loggedInUser->uid;
      }
      else {
        global $user;
        $settings['uid'] = $user->uid;
      }
    }

    // Merge body field value and format separately.
    $body = array(
      'value' => $this->randomName(32),
      'format' => filter_default_format(),
    );
    $settings['body'][$settings['langcode']][0] += $body;

    $node = new Node($settings);
    $node->save();

    // Small hack to link revisions to our test user.
    db_update('node_revision')
      ->fields(array('uid' => $node->uid))
      ->condition('vid', $node->vid)
      ->execute();
    return $node;
  }

  /**
   * Creates a custom content type based on default settings.
   *
   * @param $settings
   *   An array of settings to change from the defaults.
   *   Example: 'type' => 'foo'.
   * @return
   *   Created content type.
   */
  protected function backdropCreateContentType($settings = array()) {
    // Find a non-existent random type name.
    do {
      $name = strtolower($this->randomName(8));
    } while (node_type_get_type($name));

    // Populate defaults array.
    $defaults = array(
      'type' => $name,
      'name' => $name,
      'base' => 'node_content',
      'description' => '',
      'help' => '',
      'title_label' => 'Title',
      'has_title' => 1,
      'is_new' => TRUE,
    );
    // Imposed values for a custom type.
    $forced = array(
      'orig_type' => '',
      'old_type' => '',
      'module' => 'node',
      'custom' => 1,
      'modified' => 1,
      'locked' => 0,
    );
    $type = $forced + $settings + $defaults;
    $type = (object) $type;

    $saved_type = node_type_save($type);
    menu_rebuild();
    node_add_body_field($type);

    $this->assertEqual($saved_type, SAVED_NEW, t('Created content type %type.', array('%type' => $type->type)));

    // Reset permissions so that permissions for this content type are available.
    $this->checkPermissions(array(), TRUE);

    return $type;
  }

  /**
   * Get a list files that can be used in tests.
   *
   * @param $type
   *   File type, possible values: 'binary', 'html', 'image', 'javascript', 'php', 'sql', 'text'.
   * @param $size
   *   File size in bytes to match. Please check the tests/files folder.
   * @return
   *   List of files that match filter.
   */
  protected function backdropGetTestFiles($type, $size = NULL) {
    $files = array();
    // Make sure type is valid.
    $possible_types = array('binary', 'html', 'image', 'svg', 'javascript', 'php', 'sql', 'text');
    if (in_array($type, $possible_types)) {

      if (!in_array($type, $this->generatedTestFiles)) {
        switch ($type) {
          case 'binary':
            // Generate binary test files.
            $lines = array(64, 1024);
            $count = 0;
            foreach ($lines as $line) {
              simpletest_generate_file('binary-' . $count++, 64, $line, 'binary');
            }
            $this->generatedTestFiles[] = 'binary';
            break;

          case 'text':
            // Generate text test files.
            $lines = array(16, 256, 1024, 2048, 20480);
            $count = 0;
            foreach ($lines as $line) {
              simpletest_generate_file('text-' . $count++, 64, $line, 'text');
            }
            $this->generatedTestFiles[] = 'text';
            break;

          default:
            // Copy other test files from simpletest.
            $original = backdrop_get_path('module', 'simpletest') . '/files';
            $files = file_scan_directory($original, '/' . $type . '-.*/');
            foreach ($files as $file) {
              file_unmanaged_copy($file->uri, config_get('system.core', 'file_public_path'));
            }
            $this->generatedTestFiles[] = $type;
            break;
        }
      }

      $files = file_scan_directory('public://', '/' . $type . '\-.*/');

      // If size is set then remove any files that are not of that size.
      if ($size !== NULL) {
        foreach ($files as $file) {
          $stats = stat($file->uri);
          if ($stats['size'] != $size) {
            unset($files[$file->uri]);
          }
        }
      }
    }
    usort($files, array($this, 'backdropCompareFiles'));
    return $files;
  }

  /**
   * Compare two files based on size and file name.
   */
  protected function backdropCompareFiles($file1, $file2) {
    $compare_size = filesize($file1->uri) - filesize($file2->uri);
    if ($compare_size) {
      // Sort by file size.
      return $compare_size;
    }
    else {
      // The files were the same size, so sort alphabetically.
      return strnatcmp($file1->name, $file2->name);
    }
  }

  /**
   * Create a user with a given set of permissions.
   *
   * @param array $permissions
   *   Array of permission names to assign to user. Note that the user always
   *   has the default permissions derived from the "authenticated users" role.
   *
   * @return object|false
   *   A fully loaded user object with pass_raw property, or FALSE if account
   *   creation fails.
   */
  protected function backdropCreateUser(array $permissions = array()) {
    // Create a role with the given permission set, if any.
    $role_name = FALSE;
    if ($permissions) {
      $role_name = $this->backdropCreateRole($permissions);
      if (!$role_name) {
        return FALSE;
      }
    }

    // Create a user assigned to that role.
    $edit = array();
    $edit['name']   = $this->randomName();
    $edit['mail']   = $edit['name'] . '@example.com';
    $edit['pass']   = user_password();
    $edit['status'] = 1;
    if ($role_name) {
      $edit['roles'] = array($role_name);
    }

    $account = entity_create('user', $edit);
    $account->save();

    $this->assertTrue(!empty($account->uid), t('User created with name %name and pass %pass', array('%name' => $edit['name'], '%pass' => $edit['pass'])), t('User login'));
    if (empty($account->uid)) {
      return FALSE;
    }

    // Add the raw password so that we can log in as this user.
    $account->pass_raw = $edit['pass'];
    return $account;
  }

  /**
   * Creates a role with specified permissions.
   *
   * @param $permissions
   *   Array of permission names to assign to role.
   * @param $name
   *   (optional) String for the name of the role.  Defaults to a random string.
   * @return
   *   Role name of newly created role, or FALSE if role creation failed.
   */
  protected function backdropCreateRole(array $permissions, $name = NULL) {
    // Generate random name if it was not passed.
    if (!$name) {
      $name = $this->randomName();
    }

    // Check the all the permissions strings are valid.
    if (!$this->checkPermissions($permissions)) {
      return FALSE;
    }

    // Create new role.
    $role = new stdClass();
    $role->name = $name;
    $role->label = $name;
    user_role_save($role);
    user_role_grant_permissions($role->name, $permissions);
    $role = user_role_load($role->name);
    $this->assertTrue(isset($role->name), t('Created role of name: @name', array('@name' => $name)), t('Role'));
    if ($role && !empty($role->name)) {
      $this->assertTrue(count($role->permissions) == count($permissions), t('Created permissions: @perms', array('@perms' => implode(', ', $permissions))), t('Role'));
      return $role->name;
    }
    else {
      return FALSE;
    }
  }

  /**
   * Check to make sure that the array of permissions are valid.
   *
   * @param $permissions
   *   Permissions to check.
   * @param $reset
   *   Reset cached available permissions.
   * @return
   *   TRUE or FALSE depending on whether the permissions are valid.
   */
  protected function checkPermissions(array $permissions, $reset = FALSE) {
    $available = &backdrop_static(__FUNCTION__);

    if (!isset($available) || $reset) {
      $available = array_keys(module_invoke_all('permission'));
    }

    $valid = TRUE;
    foreach ($permissions as $permission) {
      if (!in_array($permission, $available)) {
        $this->fail(t('Invalid permission %permission.', array('%permission' => $permission)), t('Role'));
        $valid = FALSE;
      }
    }
    return $valid;
  }

  /**
   * Log in a user with the internal browser.
   *
   * If a user is already logged in, then the current user is logged out before
   * logging in the specified user.
   *
   * Please note that neither the global $user nor the passed-in user object is
   * populated with data of the logged in user. If you need full access to the
   * user object after logging in, it must be updated manually. If you also need
   * access to the plain-text password of the user (set by backdropCreateUser()),
   * e.g. to log in the same user again, then it must be re-assigned manually.
   * For example:
   * @code
   *   // Create a user.
   *   $account = $this->backdropCreateUser(array());
   *   $this->backdropLogin($account);
   *   // Load real user object.
   *   $pass_raw = $account->pass_raw;
   *   $account = user_load($account->uid);
   *   $account->pass_raw = $pass_raw;
   * @endcode
   *
   * @param User $account
   *   User object representing the user to log in.
   * @param boolean $by_email
   *   Whether to use email for login instead of username.
   *
   * @see backdropCreateUser()
   */
  protected function backdropLogin(User $account, $by_email = FALSE) {
    global $user;
    if ($this->loggedInUser) {
      $this->backdropLogout();
    }
    // Sometimes $this->loggedInUser is FALSE even when user is logged in.
    elseif ($user->uid) {
      $this->backdropLogout();
    }

    $edit = array(
      'name' => $by_email ? $account->mail : $account->name,
      'pass' => $account->pass_raw
    );
    $this->backdropPost('user/login', $edit, t('Log in'));

    // Check for the logged-in class.
    $result = $this->xpath('/html/body[contains(@class, "logged-in")]');
    $pass = $this->assertEqual(count($result), 1, t('User %name successfully logged in.', array('%name' => $account->name)), t('User login'));

    if ($pass) {
      $this->loggedInUser = $account;
    }
  }

  /**
   * Generate a token for the currently logged in user.
   */
  protected function backdropGetToken($value = '') {
    return backdrop_hmac_base64($value, $this->session_id . backdrop_get_private_key() . backdrop_get_hash_salt());
  }

  /*
   * Logs a user out of the internal browser, then check the login page to confirm logout.
   */
  protected function backdropLogout() {
    // Make a request to the logout page, and redirect to the user page, the
    // idea being if you were properly logged out you should be seeing a login
    // screen.
    $this->backdropGet('user/logout');
    $this->backdropGet('user');
    $pass = $this->assertField('name', t('Username field found.'), t('Logout'));
    $pass = $pass && $this->assertField('pass', t('Password field found.'), t('Logout'));

    if ($pass) {
      $this->loggedInUser = FALSE;
    }
  }

  /**
   * Changes the database connection to the prefixed one.
   *
   * @see BackdropWebTestCase::setUp()
   */
  protected function changeDatabasePrefix() {
    if (empty($this->databasePrefix)) {
      $this->prepareDatabasePrefix();
      // If $this->prepareDatabasePrefix() failed to work, return without
      // setting $this->setupDatabasePrefix to TRUE, so setUp() methods will
      // know to bail out.
      if (empty($this->databasePrefix)) {
        return;
      }
    }

    // Clone the current connection and replace the current prefix.
    $connection_info = Database::getConnectionInfo('default');
    Database::renameConnection('default', 'simpletest_original_default');
    foreach ($connection_info as $target => $value) {
      $connection_info[$target]['prefix'] = array(
        'default' => $value['prefix']['default'] . $this->databasePrefix,
      );
    }

    Database::addConnectionInfo('default', 'default', $connection_info['default']);

    // Indicate the database prefix was set up correctly.
    $this->setupDatabasePrefix = TRUE;
  }

  /**
   * Prepares the current environment for running the test.
   *
   * Backups various current environment variables and resets them, so they do
   * not interfere with the Backdrop site installation in which tests are executed
   * and can be restored in tearDown().
   *
   * Also sets up new resources for the testing environment, such as the public
   * filesystem and configuration directories.
   *
   * @see BackdropWebTestCase::setUp()
   * @see BackdropWebTestCase::tearDown()
   */
  protected function prepareEnvironment() {
    global $user, $language, $language_url, $settings, $config_directories;
    $site_config = config('system.core');

    // Store necessary current values before switching to prefixed database.
    $this->originalLanguage = $language;
    $this->originalLanguageUrl = $language_url;
    $this->originalConfigDirectories = $config_directories;
    $this->originalFileDirectory = $site_config->get('file_public_path');
    $this->verboseDirectoryUrl = file_create_url($this->originalFileDirectory . '/simpletest/verbose');
    $this->originalProfile = backdrop_get_profile();
    $this->originalCleanUrl = $site_config->get('clean_url');
    $this->originalUser = $user;
    $this->originalSettings = $settings;

    // Set to English to prevent exceptions from utf8_truncate() from t()
    // during install if the current language is not 'en'.
    // The following array/object conversion is copied from language_default().
    $language_url = $language = (object) array(
      'langcode' => 'en',
      'name' => 'English',
      'enabled' => 1,
      'weight' => 0,
    );

    // Save and clean the shutdown callbacks array because it is static cached
    // and will be changed by the test run. Otherwise it will contain callbacks
    // from both environments and the testing environment will try to call the
    // handlers defined by the original one.
    $callbacks = &backdrop_register_shutdown_function();
    $this->originalShutdownCallbacks = $callbacks;
    $callbacks = array();

    // Create test directory ahead of installation so fatal errors and debug
    // information can be logged during installation process.
    // Use temporary files directory with the same prefix as the database.
    $this->public_files_directory = $this->originalFileDirectory . '/simpletest/' . $this->fileDirectoryName;
    $this->private_files_directory = $this->public_files_directory . '/private';
    $this->temp_files_directory = $this->private_files_directory . '/temp';

    // Create the directories.
    file_prepare_directory($this->public_files_directory, FILE_CREATE_DIRECTORY | FILE_MODIFY_PERMISSIONS);
    file_prepare_directory($this->private_files_directory, FILE_CREATE_DIRECTORY);
    file_prepare_directory($this->temp_files_directory, FILE_CREATE_DIRECTORY);
    $this->generatedTestFiles = array();

    // Set the new config directories. During test execution, these values are
    // manually set directly in config_get_config_directory().
    $config_base_path = 'files/simpletest/' . $this->fileDirectoryName . '/config_';
    $config_directories['active'] = $config_base_path . 'active';
    $config_directories['staging'] = $config_base_path . 'staging';

    // Set the new backup directories. During test execution, these values are
    // manually set directly in backup_get_backup_directory().
    $settings['backup_directory'] = 'files/simpletest/' . $this->fileDirectoryName . '/backups';

    // Log fatal errors.
    ini_set('log_errors', 1);
    ini_set('error_log', $this->public_files_directory . '/error.log');

    // Set the test information for use in other parts of Backdrop.
    $test_info = &$GLOBALS['backdrop_test_info'];
    $test_info['test_run_id'] = $this->databasePrefix;
    $test_info['in_child_site'] = FALSE;

    // Disable Drupal compatibility for test runs.
    $settings['backdrop_drupal_compatibility'] = FALSE;

    // Indicate the environment was set up correctly.
    $this->setupEnvironment = TRUE;
  }

  /**
   * Copies the cached tables and config for a profile if one is available.
   *
   * @return boolean
   *   TRUE when cache used, FALSE when cache is not available.
   *
   * @see BackdropWebTestCase::setUp()
   * @see BackdropWebTestCase::tearDown()
   */
  protected function useCache() {
    $config_cache_dir = $this->originalFileDirectory . '/simpletest/simpletest_cache_' . $this->profile;

    if (is_dir($config_cache_dir)) {
      $prefix = 'simpletest_cache_' . $this->profile . '_';

      $tables = db_query("SHOW TABLES LIKE :prefix", array(':prefix' => db_like($prefix) . '%' ))->fetchCol();

      foreach ($tables as $table_prefix) {
        $table = substr($table_prefix, strlen($prefix));
        db_query('CREATE TABLE ' . $this->databasePrefix . $table . ' LIKE ' . $table_prefix);
        db_query('INSERT ' . $this->databasePrefix . $table . ' SELECT * FROM ' . $table_prefix);
      }

      $this->recursiveCopy($config_cache_dir, $this->public_files_directory);

      return TRUE;
    }
    return FALSE;
  }

  /**
   * Recursively copy one directory to another.
   *
   * @param string $src
   *   Source directory to copy.
   * @param string $dst
   *   Destination directory to copy files to.
   */
  private function recursiveCopy($src, $dst) {
    $dir = opendir($src);
    if (!file_exists($dst)) {
      mkdir($dst);
    }
    while (FALSE !== ($file = readdir($dir))) {
      if ($file != '.' && $file != '..' && $file != '.htaccess') {
        if (is_dir($src . '/' . $file)) {
          $this->recursiveCopy($src . '/' . $file, $dst . '/' . $file);
        }
        else {
          copy($src . '/' . $file, $dst . '/' . $file);
        }
      }
    }
    closedir($dir);
  }

  /**
   * Sets up a Backdrop site for running functional and integration tests.
   *
   * Generates a random database prefix and installs Backdrop with the specified
   * installation profile in BackdropWebTestCase::$profile into the
   * prefixed database. Afterwards, installs any additional modules specified by
   * the test.
   *
   * After installation all caches are flushed and several configuration values
   * are reset to the values of the parent site executing the test, since the
   * default values may be incompatible with the environment in which tests are
   * being executed.
   *
   * @param ...
   *   List of modules to enable for the duration of the test. This can be
   *   either a single array or a variable number of string arguments.
   *
   * @see BackdropWebTestCase::prepareDatabasePrefix()
   * @see BackdropWebTestCase::changeDatabasePrefix()
   * @see BackdropWebTestCase::prepareEnvironment()
   *
   * @return bool
   *   TRUE if set up completes, FALSE if an error occurred.
   */
  protected function setUp() {
    global $user, $language, $language_url, $conf;
    // Create the database prefix for this test.
    $this->prepareDatabasePrefix();

    // Prepare the environment for running tests.
    $this->prepareEnvironment();
    if (!$this->setupEnvironment) {
      return FALSE;
    }

    // Reset all statics and variables to perform tests in a clean environment.
    $conf = array();
    backdrop_static_reset();

    // Change the database prefix.
    // All static variables need to be reset before the database prefix is
    // changed, since BackdropCacheArray implementations attempt to
    // write back to persistent caches when they are destructed.
    $this->changeDatabasePrefix();
    if (!$this->setupDatabasePrefix) {
      return FALSE;
    }

    // This has to happen before any config changes are made to ensure that the
    // database tables from the test cache exist.
    $use_cache = $this->useCache();

    if (!$use_cache) {
      // Initialize config storage. The database storage needs to be done after
      // switching the database prefix.
      config_get_config_storage('active')->initializeStorage();
      config_get_config_storage('staging')->initializeStorage();
    }

    // Preset the 'install_profile' system variable, so the first call into
    // system_rebuild_module_data() (in backdrop_install_system()) will register
    // the test's profile as a module. Without this, the installation profile of
    // the parent site (executing the test) is registered, and the test
    // profile's hook_install() and other hook implementations are never invoked.
    config_install_default_config('system');
    config_set('system.core', 'install_profile', $this->profile);

    if (!$use_cache) {
      // Perform the actual Backdrop installation.
      include_once BACKDROP_ROOT . '/core/includes/install.inc';
      backdrop_install_system();

      // Set path variables.
      $core_config = config('system.core');
      $core_config->set('file_default_scheme', 'public');
      $core_config->set('file_public_path', $this->public_files_directory);
      $core_config->set('file_private_path', $this->private_files_directory);
      $core_config->set('file_temporary_path', $this->temp_files_directory);
      $core_config->save();

      // Ensure schema versions are recalculated.
      backdrop_static_reset('backdrop_get_schema_versions');

      // Include the testing profile.
      config_set('system.core', 'install_profile', $this->profile);
      $profile_details = install_profile_info($this->profile, 'en');

      // Install the modules specified by the testing profile.
      module_enable($profile_details['dependencies'], FALSE);
      // Run the profile tasks.
      $install_profile_module_exists = db_query("SELECT 1 FROM {system} WHERE type = 'module' AND name = :name", array(':name' => $this->profile))->fetchField();
      if ($install_profile_module_exists) {
        module_enable(array($this->profile), FALSE);
      }
    }
    else {
      // Set path variables. This must be done even when using a profile cache.
      $core_config = config('system.core');
      $core_config->set('file_default_scheme', 'public');
      $core_config->set('file_public_path', $this->public_files_directory);
      $core_config->set('file_private_path', $this->private_files_directory);
      $core_config->set('file_temporary_path', $this->temp_files_directory);
      $core_config->save();
    }

    // Set 'parent_profile' of simpletest to add the parent profile's
    // search path to the child site's search paths.
    // @see backdrop_system_listing()
    // @todo This may need to be primed like 'install_profile' above.
    config_set('simpletest.settings', 'parent_profile', $this->originalProfile);

    // Install modules needed for this test. This could have been passed in as
    // either a single array argument or a variable number of string arguments.
    // @todo Remove this compatibility layer and only accept a single array.
    $modules = func_get_args();
    if (isset($modules[0]) && is_array($modules[0])) {
      $modules = $modules[0];
    }
    if ($modules) {
      $success = module_enable($modules, TRUE);
      $this->assertTrue($success, t('Enabled modules: %modules', array('%modules' => implode(', ', $modules))));
    }

    // Reset/rebuild all data structures after enabling the modules.
    $this->resetAll();

    // Ensure that the session is not written to the new environment and replace
    // the global $user session with uid 1 from the new test site.
    backdrop_save_session(FALSE);
    // Login as uid 1.
    $user = user_load(1);

    // Restore necessary variables.
    state_set('install_task', 'done');
    config_set('system.core', 'clean_url', $this->originalCleanUrl);
    config_set('system.core', 'site_mail', 'simpletest@example.com');
    config_set('system.date', 'date_default_timezone', date_default_timezone_get());
    backdrop_static_reset('url');

    // Set up English language.
    unset($conf['language_default']);
    $language_url = $language = language_default();

    // Use the test mail class instead of the default mail handler class.
    config_set('system.mail', 'default-system', 'TestingMailSystem');

    // Disable news checking against BackdropCMS.org.
    $dashboard_config = config('dashboard.settings');
    if (!$dashboard_config->isNew()) {
      $dashboard_config->set('news_feed_url', FALSE);
      $dashboard_config->save();
    }

    // Run cron once in that environment, as install.php does at the end of
    // the installation process.
    backdrop_cron_run();

    backdrop_set_time_limit($this->timeLimit);
    $this->setup = TRUE;
    return TRUE;
  }

  /**
   * Reset all data structures after having enabled new modules.
   *
   * This method is called by BackdropWebTestCase::setUp() after enabling
   * the requested modules. It must be called again when additional modules
   * are enabled later.
   */
  protected function resetAll() {
    // Reset all static variables.
    backdrop_static_reset();
    // Reset the list of enabled modules.
    module_list(TRUE);

    // Reset cached schema for new database prefix. This must be done before
    // backdrop_flush_all_caches() so rebuilds can make use of the schema of
    // modules enabled on the cURL side.
    backdrop_get_schema(NULL, TRUE);

    // Perform rebuilds and flush remaining caches.
    backdrop_flush_all_caches();

    // Reload global $conf array and permissions.
    $this->refreshVariables();
    $this->checkPermissions(array(), TRUE);
  }

  /**
   * Refresh the in-memory set of variables and state values. Useful after a
   * page request is made that changes a variable in a different thread.
   *
   * In other words calling a settings page with $this->backdropPost() with a changed
   * value would update a variable to reflect that change, but in the thread that
   * made the call (thread running the test) the changed variable would not be
   * picked up.
   *
   * This method clears the variables cache and loads a fresh copy from the database
   * to ensure that the most up-to-date set of variables is loaded.
   */
  protected function refreshVariables() {
    global $conf;
    cache('bootstrap')->delete('variables');
    $conf = variable_initialize();
    backdrop_static_reset('states');
    backdrop_static_reset('config');
  }

  /**
   * Delete created files and temporary files directory, delete the tables
   * created by setUp(), and reset the database prefix.
   */
  protected function tearDown() {
    global $user, $language, $language_url, $settings, $config_directories;

    // In case a fatal error occurred that was not in the test process read the
    // log to pick up any fatal errors.
    simpletest_log_read($this->testId, $this->databasePrefix, get_class($this), TRUE);

    $emailCount = count(state_get('test_email_collector', array()));
    if ($emailCount) {
      $message = format_plural($emailCount, '1 email was sent during this test.', '@count emails were sent during this test.');
      $this->pass($message, t('Email'));
    }

    // Reset static variables, this may flush pending writes to the theme
    // registry as well.
    backdrop_static_reset();

    // Delete temporary files directory.
    file_unmanaged_delete_recursive($this->originalFileDirectory . '/simpletest/' . $this->fileDirectoryName);

    // Remove all prefixed tables.
    $connection_info = Database::getConnectionInfo('default');
    $tables = db_find_tables($connection_info['default']['prefix']['default'] . '%');
    if (empty($tables)) {
      $this->fail('Failed to find test tables to drop.');
    }
    $prefix_length = strlen($connection_info['default']['prefix']['default']);
    foreach ($tables as $table) {
      if (db_drop_table(substr($table, $prefix_length))) {
        unset($tables[$table]);
      }
    }
    if (!empty($tables)) {
      $this->fail('Failed to drop all prefixed tables.');
    }

    // In PHP 8 some tests encounter problems as some shutdown code tries to
    // access the database connection after it's been explicitly closed (for
    // example the destructor of BackdropCacheArray). We avoid this by not fully
    // destroying the test database connection.
    $close = \PHP_VERSION_ID < 80000;

    // Get back to the original connection.
    Database::removeConnection('default', $close);
    Database::renameConnection('simpletest_original_default', 'default');

    // Delete the database table prefix record.
    db_delete('simpletest_prefix')
      ->condition('test_id', $this->testId)
      ->condition('prefix', $this->databasePrefix)
      ->execute();

    // Set the configuration directories back to the originals.
    $config_directories = $this->originalConfigDirectories;

    // Restore the original settings.
    $settings = $this->originalSettings;

    // Restore original shutdown callbacks array to prevent original
    // environment of calling handlers from test run.
    $callbacks = &backdrop_register_shutdown_function();
    $callbacks = $this->originalShutdownCallbacks;

    // Return the user to the original one.
    $user = $this->originalUser;
    backdrop_save_session(TRUE);

    // Ensure that internal logged in variable and cURL options are reset.
    $this->loggedInUser = FALSE;
    $this->additionalCurlOptions = array();

    // Reload module list and implementations to ensure that test module hooks
    // aren't called after tests.
    module_list(TRUE);
    module_implements_reset();

    // Rebuild caches.
    $this->refreshVariables();

    // Reset language.
    $language = $this->originalLanguage;
    $language_url = $this->originalLanguageUrl;

    // Close the CURL handler and reset the cookies array, so that test classes
    // containing multiple tests are not polluted.
    $this->curlClose();
    $this->cookies = array();
  }

  /**
   * Initializes the cURL connection.
   *
   * If the simpletest_httpauth_credentials variable is set, this function will
   * add HTTP authentication headers. This is necessary for testing sites that
   * are protected by login credentials from public access.
   * See the description of $curl_options for other options.
   */
  protected function curlInitialize() {
    global $base_url;

    if (!isset($this->curlHandle)) {
      $this->curlHandle = curl_init();

      // Some versions/configurations of cURL break on a NULL cookie jar, so
      // supply a real file.
      if (empty($this->cookieFile)) {
        $this->cookieFile = $this->public_files_directory . '/cookie.jar';
      }

      $curl_options = array(
        CURLOPT_COOKIEJAR => $this->cookieFile,
        CURLOPT_URL => $base_url,
        CURLOPT_FOLLOWLOCATION => FALSE,
        CURLOPT_RETURNTRANSFER => TRUE,
        CURLOPT_SSL_VERIFYPEER => FALSE, // Required to make the tests run on HTTPS.
        CURLOPT_SSL_VERIFYHOST => FALSE, // Required to make the tests run on HTTPS.
        CURLOPT_HEADERFUNCTION => array(&$this, 'curlHeaderCallback'),
        CURLOPT_USERAGENT => $this->databasePrefix,
      );
      if (isset($this->httpauth_credentials)) {
        $curl_options[CURLOPT_HTTPAUTH] = $this->httpauth_method;
        $curl_options[CURLOPT_USERPWD] = $this->httpauth_credentials;
      }
      // curl_setopt_array() returns FALSE if any of the specified options
      // cannot be set, and stops processing any further options.
      $result = curl_setopt_array($this->curlHandle, $this->additionalCurlOptions + $curl_options);
      if (!$result) {
        throw new Exception('One or more cURL options could not be set.');
      }

      // By default, the child session name should be the same as the parent.
      $this->session_name = session_name();
    }
    // We set the user agent header on each request so as to use the current
    // time and a new unique ID.
    if (preg_match('/simpletest\d+/', $this->databasePrefix, $matches)) {
      curl_setopt($this->curlHandle, CURLOPT_USERAGENT, backdrop_generate_test_ua($matches[0]));
    }
  }

  /**
   * Initializes and executes a cURL request.
   *
   * @param $curl_options
   *   An associative array of cURL options to set, where the keys are constants
   *   defined by the cURL library. For a list of valid options, see
   *   http://www.php.net/manual/function.curl-setopt.php
   * @param $redirect
   *   FALSE if this is an initial request, TRUE if this request is the result
   *   of a redirect.
   *
   * @return
   *   The content returned from the call to curl_exec().
   *
   * @see curlInitialize()
   */
  protected function curlExec($curl_options, $redirect = FALSE) {
    $this->curlInitialize();

    // cURL incorrectly handles URLs with a fragment by including the
    // fragment in the request to the server, causing some web servers
    // to reject the request citing "400 - Bad Request". To prevent
    // this, we strip the fragment from the request.
    // TODO: Remove this; fixed in curl 7.20.0.
    if (!empty($curl_options[CURLOPT_URL]) && strpos($curl_options[CURLOPT_URL], '#')) {
      $original_url = $curl_options[CURLOPT_URL];
      $curl_options[CURLOPT_URL] = strtok($curl_options[CURLOPT_URL], '#');
    }

    $url = empty($curl_options[CURLOPT_URL]) ? curl_getinfo($this->curlHandle, CURLINFO_EFFECTIVE_URL) : $curl_options[CURLOPT_URL];

    if (!empty($curl_options[CURLOPT_POST])) {
      // This is a fix for the Curl library to prevent Expect: 100-continue
      // headers in POST requests, that may cause unexpected HTTP response
      // codes from some web servers (like lighttpd that returns a 417 error
      // code). It is done by setting an empty "Expect" header field that is
      // not overwritten by Curl.
      $curl_options[CURLOPT_HTTPHEADER][] = 'Expect:';
    }
    curl_setopt_array($this->curlHandle, $this->additionalCurlOptions + $curl_options);

    if (!$redirect) {
      // Reset headers, the session ID and the redirect counter.
      $this->session_id = NULL;
      $this->headers = array();
      $this->redirect_count = 0;
    }

    $content = curl_exec($this->curlHandle);
    $status = curl_getinfo($this->curlHandle, CURLINFO_HTTP_CODE);

    // cURL incorrectly handles URLs with fragments, so instead of
    // letting cURL handle redirects we take of them ourselves to
    // to prevent fragments being sent to the web server as part
    // of the request.
    // TODO: Remove this; fixed in curl 7.20.0.
    if (in_array($status, array(300, 301, 302, 303, 305, 307)) && $this->redirect_count < $this->maximumRedirects) {
      if ($this->backdropGetHeader('location')) {
        $this->redirect_count++;
        $curl_options = array();
        $curl_options[CURLOPT_URL] = $this->backdropGetHeader('location');
        $curl_options[CURLOPT_HTTPGET] = TRUE;
        return $this->curlExec($curl_options, TRUE);
      }
    }

    $this->backdropSetContent($content, isset($original_url) ? $original_url : curl_getinfo($this->curlHandle, CURLINFO_EFFECTIVE_URL));
    $message_vars = array(
      '!method' => !empty($curl_options[CURLOPT_NOBODY]) ? 'HEAD' : (empty($curl_options[CURLOPT_POSTFIELDS]) ? 'GET' : 'POST'),
      '@url' => isset($original_url) ? $original_url : $url,
      '@status' => $status,
      '!length' => format_size(strlen($this->backdropGetContent()))
    );
    $message = t('!method @url returned @status (!length).', $message_vars);
    $this->assertTrue($this->backdropGetContent() !== FALSE, $message, t('Browser'));
    return $this->backdropGetContent();
  }

  /**
   * Reads headers and registers errors received from the tested site.
   *
   * @see _backdrop_log_error().
   *
   * @param $curlHandler
   *   The cURL handler.
   * @param $header
   *   An header.
   *
   * @return int
   *   The header length in bytes.
   */
  protected function curlHeaderCallback($curlHandler, $header) {
    // Header fields can be extended over multiple lines by preceding each
    // extra line with at least one SP or HT. They should be joined on receive.
    // Details are in RFC2616 section 4.
    if ($header[0] == ' ' || $header[0] == "\t") {
      // Normalize whitespace between chucks.
      $this->headers[] = array_pop($this->headers) . ' ' . trim($header);
    }
    else {
      $this->headers[] = $header;
    }

    // Errors are being sent via X-Backdrop-Assertion-* headers,
    // generated by _backdrop_log_error() in the exact form required
    // by BackdropWebTestCase::error().
    if (preg_match('/^X-Backdrop-Assertion-[0-9]+: (.*)$/', trim($header), $matches)) {
      // Call BackdropWebTestCase::error() with the parameters from the header.
      call_user_func_array(array(&$this, 'error'), unserialize(urldecode($matches[1])));
    }

    // Save cookies.
    if (preg_match('/^Set-Cookie: ([^=]+)=(.+)/', $header, $matches)) {
      $name = $matches[1];
      $parts = array_map('trim', explode(';', $matches[2]));
      $value = array_shift($parts);
      $this->cookies[$name] = array('value' => $value, 'secure' => in_array('secure', $parts));
      if ($name == $this->session_name) {
        if ($value != 'deleted') {
          $this->session_id = $value;
        }
        else {
          $this->session_id = NULL;
        }
      }
    }

    // This is required by cURL.
    return strlen($header);
  }

  /**
   * Close the cURL handler and unset the handler.
   */
  protected function curlClose() {
    if (isset($this->curlHandle)) {
      curl_close($this->curlHandle);
      unset($this->curlHandle);
    }
  }

  /**
   * Parse content returned from curlExec using DOM and SimpleXML.
   *
   * @return
   *   A SimpleXMLElement or FALSE on failure.
   */
  protected function parse() {
    if (!$this->elements) {
      $htmlDom = new DOMDocument();
      // DOM can load HTML soup, which can throw warnings. Suppress them here.
      @$htmlDom->loadHTML('<?xml encoding="UTF-8">' . $this->backdropGetContent());
      if ($htmlDom) {
        $this->pass(t('Valid HTML found on "@path"', array('@path' => $this->getUrl())), t('Browser'));
        // Import our DOM tree.
        $this->elements = simplexml_import_dom($htmlDom);
      }
    }
    if (!$this->elements) {
      $this->fail(t('Parsed page successfully.'), t('Browser'));
    }

    return $this->elements;
  }

  /**
   * Retrieves a Backdrop path or an absolute path.
   *
   * @param $path
   *   Backdrop path or URL to load into internal browser
   * @param $options
   *   Options to be forwarded to url().
   * @param $headers
   *   An array containing additional HTTP request headers, each formatted as
   *   "name: value".
   * @return
   *   The retrieved HTML string, also available as $this->backdropGetContent()
   */
  protected function backdropGet($path, array $options = array(), array $headers = array()) {
    $options['absolute'] = TRUE;

    // We re-using a CURL connection here. If that connection still has certain
    // options set, it might change the GET into a POST. Make sure we clear out
    // previous options.
    $out = $this->curlExec(array(CURLOPT_HTTPGET => TRUE, CURLOPT_URL => url($path, $options), CURLOPT_NOBODY => FALSE, CURLOPT_HTTPHEADER => $headers));
    $this->refreshVariables(); // Ensure that any changes to variables in the other thread are picked up.

    // Replace original page output with new output from redirected page(s).
    if ($new = $this->checkForMetaRefresh()) {
      $out = $new;
    }
    $this->verbose('GET request to: ' . $path .
                   '<hr />Ending URL: ' . $this->getUrl() .
                   '<hr />' . $out);
    return $out;
  }

  /**
   * Retrieve a Backdrop path or an absolute path and JSON decode the result.
   */
  protected function backdropGetAJAX($path, array $options = array(), array $headers = array()) {
    $headers[] = 'X-Requested-With: XMLHttpRequest';
    $headers[] = 'Accept: application/vnd.backdrop-ajax, */*; q=0.01';
    return backdrop_json_decode($this->backdropGet($path, $options, $headers));
  }

  /**
   * Execute a POST request on a Backdrop page.
   * It will be done as usual POST request with SimpleBrowser.
   *
   * @param $path
   *   Location of the post form. Either a Backdrop path or an absolute path or
   *   NULL to post to the current page. For multi-stage forms you can set the
   *   path to NULL and have it post to the last received page. Example:
   *
   *   @code
   *   // First step in form.
   *   $edit = array(...);
   *   $this->backdropPost('some_url', $edit, t('Save'));
   *
   *   // Second step in form.
   *   $edit = array(...);
   *   $this->backdropPost(NULL, $edit, t('Save'));
   *   @endcode
   * @param  $edit
   *   Field data in an associative array. Changes the current input fields
   *   (where possible) to the values indicated. A checkbox can be set to
   *   TRUE to be checked and FALSE to be unchecked. Note that when a form
   *   contains file upload fields, other fields cannot start with the '@'
   *   character.
   *
   *   Multiple select fields can be set using name[] and setting each of the
   *   possible values. Example:
   *   @code
   *   $edit = array();
   *   $edit['name[]'] = array('value1', 'value2');
   *   @endcode
   * @param $submit
   *   Value of the submit button whose click is to be emulated. For example,
   *   t('Save'). The processing of the request depends on this value. For
   *   example, a form may have one button with the value t('Save') and another
   *   button with the value t('Delete'), and execute different code depending
   *   on which one is clicked.
   *
   *   This function can also be called to emulate an Ajax submission. In this
   *   case, this value needs to be an array with the following keys:
   *   - path: A path to submit the form values to for Ajax-specific processing,
   *     which is likely different than the $path parameter used for retrieving
   *     the initial form. Defaults to 'system/ajax'.
   *   - triggering_element: If the value for the 'path' key is 'system/ajax' or
   *     another generic Ajax processing path, this needs to be set to the name
   *     of the element. If the name doesn't identify the element uniquely, then
   *     this should instead be an array with a single key/value pair,
   *     corresponding to the element name and value. The callback for the
   *     generic Ajax processing path uses this to find the #ajax information
   *     for the element, including which specific callback to use for
   *     processing the request.
   *
   *   This can also be set to NULL in order to emulate an Internet Explorer
   *   submission of a form with a single text field, and pressing ENTER in that
   *   textfield: under these conditions, no button information is added to the
   *   POST data.
   * @param $options
   *   Options to be forwarded to url().
   * @param $headers
   *   An array containing additional HTTP request headers, each formatted as
   *   "name: value".
   * @param $form_html_id
   *   (optional) HTML ID of the form to be submitted. On some pages
   *   there are many identical forms, so just using the value of the submit
   *   button is not enough. For example: 'trigger-node-presave-assign-form'.
   *   Note that this is not the Backdrop $form_id, but rather the HTML ID of
   *   the form, which is typically the same thing but with hyphens replacing
   *   the underscores.
   * @param $extra_post
   *   (optional) A string of additional data to append to the POST submission.
   *   This can be used to add POST data for which there are no HTML fields, as
   *   is done by backdropPostAJAX(). This string is literally appended to the
   *   POST data, so it must already be urlencoded and contain a leading "&"
   *   (e.g., "&extra_var1=hello+world&extra_var2=you%26me").
   *
   * @return string|FALSE
   *   The returned HTML content from the response.
   */
  protected function backdropPost($path, $edit, $submit, array $options = array(), array $headers = array(), $form_html_id = NULL, $extra_post = NULL) {
    $submit_matches = FALSE;
    $ajax = is_array($submit);
    if (isset($path)) {
      $this->backdropGet($path, $options);
    }
    if ($this->parse()) {
      $edit_save = $edit;
      // Let's iterate over all the forms.
      $xpath = "//form";
      if (!empty($form_html_id)) {
        $xpath .= "[@id='" . $form_html_id . "']";
      }
      $forms = $this->xpath($xpath);
      foreach ($forms as $form) {
        // We try to set the fields of this form as specified in $edit.
        $edit = $edit_save;
        $post = array();
        $upload = array();
        $submit_matches = $this->handleForm($post, $edit, $upload, $ajax ? NULL : $submit, $form);
        $action = isset($form['action']) ? $this->getAbsoluteUrl((string) $form['action']) : $this->getUrl();
        if ($ajax) {
          $action = $this->getAbsoluteUrl(!empty($submit['path']) ? $submit['path'] : 'system/ajax');
          // Ajax callbacks verify the triggering element if necessary, so while
          // we may eventually want extra code that verifies it in the
          // handleForm() function, it's not currently a requirement.
          $submit_matches = TRUE;
        }

        // We post only if we managed to handle every field in edit and the
        // submit button matches.
        if (!$edit && ($submit_matches || !isset($submit))) {
          $post_array = $post;
          if ($upload) {
            // TODO: cURL handles file uploads for us, but the implementation
            // is broken. This is a less than elegant workaround. Alternatives
            // are being explored at #253506.
            foreach ($upload as $key => $file) {
              $file = backdrop_realpath($file);
              if ($file && is_file($file)) {
                $post[$key] = curl_file_create($file);
              }
            }
          }
          else {
            foreach ($post as $key => $value) {
              // Encode according to application/x-www-form-urlencoded
              // Both names and values needs to be urlencoded, according to
              // http://www.w3.org/TR/html4/interact/forms.html#h-17.13.4.1
              $post[$key] = urlencode($key) . '=' . urlencode($value);
            }
            $post = implode('&', $post) . $extra_post;
          }
          $out = $this->curlExec(array(CURLOPT_URL => $action, CURLOPT_POST => TRUE, CURLOPT_POSTFIELDS => $post, CURLOPT_HTTPHEADER => $headers));
          // Ensure that any changes to variables in the other thread are picked up.
          $this->refreshVariables();

          // Replace original page output with new output from redirected page(s).
          if ($new = $this->checkForMetaRefresh()) {
            $out = $new;
          }
          $this->verbose('POST request to: ' . $path .
                         '<hr />Ending URL: ' . ($this->getUrl()) .
                         '<hr />Fields: ' . highlight_string('<?php ' . var_export($post_array, TRUE), TRUE) .
                         '<hr />' . $out);
          return $out;
        }
      }
      // We have not found a form which contained all fields of $edit.
      foreach ($edit as $name => $value) {
        $this->fail(t('Failed to set field @name to @value', array('@name' => $name, '@value' => $value)));
      }
      if (!$ajax && isset($submit)) {
        $this->assertTrue($submit_matches, t('Found the @submit button', array('@submit' => $submit)));
      }
      $this->fail(t('Found the requested form fields at @path', array('@path' => $path)));
    }
    return FALSE;
  }

  /**
   * Execute an Ajax submission.
   *
   * This executes a POST as ajax.js does. It uses the returned JSON data, an
   * array of commands, to update $this->content using equivalent DOM
   * manipulation as is used by ajax.js. It also returns the array of commands.
   *
   * @param $path
   *   Location of the form containing the Ajax enabled element to test. Can be
   *   either a Backdrop path or an absolute path or NULL to use the current
   *   page.
   * @param $edit
   *   Field data in an associative array. Changes the current input fields
   *   (where possible) to the values indicated.
   * @param $triggering_element
   *   The name of the form element that is responsible for triggering the Ajax
   *   functionality to test. May be a string or, if the triggering element is
   *   a button, an associative array where the key is the name of the button
   *   and the value is the button label. i.e.) array('op' => t('Refresh')).
   * @param $ajax_path
   *   (optional) Override the path set by the Ajax settings of the triggering
   *   element. In the absence of both the triggering element's Ajax path and
   *   $ajax_path 'system/ajax' will be used.
   * @param $options
   *   (optional) Options to be forwarded to url().
   * @param $headers
   *   (optional) An array containing additional HTTP request headers, each
   *   formatted as "name: value". Forwarded to backdropPost().
   * @param $form_html_id
   *   (optional) HTML ID of the form to be submitted, use when there is more
   *   than one identical form on the same page and the value of the triggering
   *   element is not enough to identify the form. Note this is not the Backdrop
   *   ID of the form but rather the HTML ID of the form.
   * @param $ajax_settings
   *   (optional) An array of Ajax settings which if specified will be used in
   *   place of the Ajax settings of the triggering element.
   *
   * @return
   *   An array of Ajax commands.
   *
   * @see backdropPost()
   * @see ajax.js
   */
  protected function backdropPostAJAX($path, $edit, $triggering_element, $ajax_path = NULL, array $options = array(), array $headers = array(), $form_html_id = NULL, $ajax_settings = NULL) {
    // Get the content of the initial page prior to calling backdropPost(), since
    // backdropPost() replaces $this->content.
    if (isset($path)) {
      $this->backdropGet($path, $options);
    }
    $content = $this->content;
    $backdrop_settings = $this->backdropSettings;

    // Get the Ajax settings bound to the triggering element.
    if (!isset($ajax_settings)) {
      if (is_array($triggering_element)) {
        $xpath = '//*[@name="' . key($triggering_element) . '" and @value="' . current($triggering_element) . '"]';
      }
      else {
        $xpath = '//*[@name="' . $triggering_element . '"]';
      }
      if (isset($form_html_id)) {
        $xpath = '//form[@id="' . $form_html_id . '"]' . $xpath;
      }
      $element = $this->xpath($xpath);
      $element_id = (string) $element[0]['id'];
      $ajax_settings = $backdrop_settings['ajax'][$element_id];
    }

    // Add extra information to the POST data as ajax.js does.
    $extra_post = '';
    if (isset($ajax_settings['submit'])) {
      foreach ($ajax_settings['submit'] as $key => $value) {
        $extra_post .= '&' . urlencode($key) . '=' . urlencode($value);
      }
    }
    foreach ($this->xpath('//*[@id]') as $element) {
      $id = (string) $element['id'];
      $extra_post .= '&' . urlencode('ajax_html_ids[]') . '=' . urlencode($id);
    }
    if (isset($backdrop_settings['ajaxPageState'])) {
      $extra_post .= '&' . urlencode('ajax_page_state[theme]') . '=' . urlencode($backdrop_settings['ajaxPageState']['theme']);
      $extra_post .= '&' . urlencode('ajax_page_state[theme_token]') . '=' . urlencode($backdrop_settings['ajaxPageState']['theme_token']);
      foreach ($backdrop_settings['ajaxPageState']['css'] as $key => $value) {
        $extra_post .= '&' . urlencode("ajax_page_state[css][$key]") . '=1';
      }
      foreach ($backdrop_settings['ajaxPageState']['js'] as $key => $value) {
        $extra_post .= '&' . urlencode("ajax_page_state[js][$key]") . '=1';
      }
    }

    // Unless a particular path is specified, use the one specified by the
    // Ajax settings, or else 'system/ajax'.
    if (!isset($ajax_path)) {
      $ajax_path = isset($ajax_settings['url']) ? $ajax_settings['url'] : 'system/ajax';
    }

    // Submit the POST request.
    $headers[] = 'X-Requested-With: XMLHttpRequest';
    $headers[] = 'Accept: application/vnd.backdrop-ajax, */*; q=0.01';
    $return = backdrop_json_decode($this->backdropPost(NULL, $edit, array('path' => $ajax_path, 'triggering_element' => $triggering_element), $options, $headers, $form_html_id, $extra_post));
    $this->assertIdentical($this->backdropGetHeader('X-Backdrop-Ajax-Token'), '1', 'Ajax response header found.');

    // Change the page content by applying the returned commands.
    if (!empty($ajax_settings) && !empty($return)) {
      // ajax.js applies some defaults to the settings object, so do the same
      // for what's used by this function.
      $ajax_settings += array(
        'method' => 'replaceWith',
      );
      // DOM can load HTML soup. But, HTML soup can throw warnings, suppress
      // them.
      $dom = new DOMDocument();
      @$dom->loadHTML($content);
      // XPath allows for finding wrapper nodes better than DOM does.
      $xpath = new DOMXPath($dom);
      foreach ($return as $command) {
        switch ($command['command']) {
          case 'settings':
            $backdrop_settings = backdrop_array_merge_deep($backdrop_settings, $command['settings']);
            break;

          case 'insert':
            $wrapperNode = NULL;
            // When a command doesn't specify a selector, use the
            // #ajax['wrapper'] which is always an HTML ID.
            if (!isset($command['selector'])) {
              $wrapperNode = $xpath->query('//*[@id="' . $ajax_settings['wrapper'] . '"]')->item(0);
            }
            // @todo Ajax commands can target any jQuery selector, but these are
            //   hard to fully emulate with XPath. For now, just handle 'head'
            //   and 'body', since these are used by ajax_render().
            elseif (in_array($command['selector'], array('head', 'body'))) {
              $wrapperNode = $xpath->query('//' . $command['selector'])->item(0);
            }
            if ($wrapperNode) {
              // ajax.js adds an enclosing DIV to work around a Safari bug.
              $newDom = new DOMDocument();
              // DOM can load HTML soup. But, HTML soup can throw warnings,
              // suppress them.
              $newDom->loadHTML('<div>' . $command['data'] . '</div>');
              // Suppress warnings thrown when duplicate HTML IDs are
              // encountered. This probably means we are replacing an element
              // with the same ID.
              $newNode = @$dom->importNode($newDom->documentElement->firstChild->firstChild, TRUE);
              $method = isset($command['method']) ? $command['method'] : $ajax_settings['method'];
              // The "method" is a jQuery DOM manipulation function. Emulate
              // each one using PHP's DOMNode API.
              switch ($method) {
                case 'replaceWith':
                  $wrapperNode->parentNode->replaceChild($newNode, $wrapperNode);
                  break;
                case 'append':
                  $wrapperNode->appendChild($newNode);
                  break;
                case 'prepend':
                  // If no firstChild, insertBefore() falls back to
                  // appendChild().
                  $wrapperNode->insertBefore($newNode, $wrapperNode->firstChild);
                  break;
                case 'before':
                  $wrapperNode->parentNode->insertBefore($newNode, $wrapperNode);
                  break;
                case 'after':
                  // If no nextSibling, insertBefore() falls back to
                  // appendChild().
                  $wrapperNode->parentNode->insertBefore($newNode, $wrapperNode->nextSibling);
                  break;
                case 'html':
                  foreach ($wrapperNode->childNodes as $childNode) {
                    $wrapperNode->removeChild($childNode);
                  }
                  $wrapperNode->appendChild($newNode);
                  break;
              }
            }
            break;

          case 'updateBuildId':
            $buildId = $xpath->query('//input[@name="form_build_id" and @value="' . $command['old'] . '"]')->item(0);
            if ($buildId) {
              $buildId->setAttribute('value', $command['new']);
            }
            break;

          // @todo Add suitable implementations for these commands in order to
          //   have full test coverage of what ajax.js can do.
          case 'remove':
            break;
          case 'changed':
            break;
          case 'css':
            break;
          case 'data':
            break;
          case 'restripe':
            break;
          case 'addCss':
            break;
        }
      }
      $content = $dom->saveHTML();
    }
    $this->backdropSetContent($content);
    $this->backdropSetSettings($backdrop_settings);

    $verbose = 'AJAX POST request to: ' . $path;
    $verbose .= '<br />AJAX callback path: ' . $ajax_path;
    $verbose .= '<hr />Ending URL: ' . $this->getUrl();
    $verbose .= '<hr />' . $this->content;

    $this->verbose($verbose);
    return $return;
  }

  /**
   * Runs cron in the Backdrop installed by SimpleTest.
   */
  protected function cronRun() {
    $this->backdropGet($GLOBALS['base_url'] . '/core/cron.php', array('external' => TRUE, 'query' => array('cron_key' => state_get('cron_key'))));
  }

  /**
   * Check for meta refresh tag and if found call backdropGet() recursively. This
   * function looks for the http-equiv attribute to be set to "Refresh"
   * and is case-sensitive.
   *
   * @return
   *   Either the new page content or FALSE.
   */
  protected function checkForMetaRefresh() {
    if (strpos($this->backdropGetContent(), '<meta ') && $this->parse()) {
      $refresh = $this->xpath('//meta[@http-equiv="Refresh"]');
      if (!empty($refresh)) {
        // Parse the content attribute of the meta tag for the format:
        // "[delay]: URL=[page_to_redirect_to]".
        if (preg_match('/\d+;\s*URL=(?P<url>.*)/i', $refresh[0]['content'], $match)) {
          return $this->backdropGet($this->getAbsoluteUrl(decode_entities($match['url'])));
        }
      }
    }
    return FALSE;
  }

  /**
   * Retrieves only the headers for a Backdrop path or an absolute path.
   *
   * @param $path
   *   Backdrop path or URL to load into internal browser
   * @param $options
   *   Options to be forwarded to url().
   * @param $headers
   *   An array containing additional HTTP request headers, each formatted as
   *   "name: value".
   * @return
   *   The retrieved headers, also available as $this->backdropGetContent()
   */
  protected function backdropHead($path, array $options = array(), array $headers = array()) {
    $options['absolute'] = TRUE;
    $out = $this->curlExec(array(CURLOPT_NOBODY => TRUE, CURLOPT_URL => url($path, $options), CURLOPT_HTTPHEADER => $headers));
    $this->refreshVariables(); // Ensure that any changes to variables in the other thread are picked up.
    return $out;
  }

  /**
   * Handle form input related to backdropPost(). Ensure that the specified fields
   * exist and attempt to create POST data in the correct manner for the particular
   * field type.
   *
   * @param $post
   *   Reference to array of post values.
   * @param $edit
   *   Reference to array of edit values to be checked against the form.
   * @param $submit
   *   Form submit button value.
   * @param SimpleXmlElement $form
   *   A SimpleXmlElement containing the form.
   * @return
   *   Submit value matches a valid submit input in the form.
   */
  protected function handleForm(&$post, &$edit, &$upload, $submit, $form) {
    // Retrieve the form elements.
    $elements = $form->xpath('.//input[not(@disabled)]|.//textarea[not(@disabled)]|.//select[not(@disabled)]');
    $submit_matches = FALSE;
    foreach ($elements as $element) {
      // SimpleXML objects need string casting all the time.
      $name = (string) $element['name'];
      // This can either be the type of <input> or the name of the tag itself
      // for <select> or <textarea>.
      $type = isset($element['type']) ? (string) $element['type'] : $element->getName();
      $value = isset($element['value']) ? (string) $element['value'] : '';
      $done = FALSE;
      if (isset($edit[$name])) {
        switch ($type) {
          case 'color':
          case 'email':
          case 'hidden':
          case 'number':
          case 'range':
          case 'text':
          case 'tel':
          case 'date':
          case 'time':
          case 'textarea':
          case 'url':
          case 'password':
          case 'search':
            $post[$name] = $edit[$name];
            unset($edit[$name]);
            break;
          case 'radio':
            if ($edit[$name] == $value) {
              $post[$name] = $edit[$name];
              unset($edit[$name]);
            }
            break;
          case 'checkbox':
            // To prevent checkbox from being checked.pass in a FALSE,
            // otherwise the checkbox will be set to its value regardless
            // of $edit.
            if ($edit[$name] === FALSE) {
              unset($edit[$name]);
              continue 2;
            }
            else {
              unset($edit[$name]);
              $post[$name] = $value;
            }
            break;
          case 'select':
            $new_value = $edit[$name];
            $options = $this->getAllOptions($element);
            if (is_array($new_value)) {
              // Multiple select box.
              if (!empty($new_value)) {
                $index = 0;
                $key = preg_replace('/\[\]$/', '', $name);
                foreach ($options as $option) {
                  $option_value = (string) $option['value'];
                  if (in_array($option_value, $new_value)) {
                    $post[$key . '[' . $index++ . ']'] = $option_value;
                    $done = TRUE;
                    unset($edit[$name]);
                  }
                }
              }
              else {
                // No options selected: do not include any POST data for the
                // element.
                $done = TRUE;
                unset($edit[$name]);
              }
            }
            else {
              // Single select box.
              foreach ($options as $option) {
                if ($new_value == $option['value']) {
                  $post[$name] = $new_value;
                  unset($edit[$name]);
                  $done = TRUE;
                  break;
                }
              }
            }
            break;
          case 'file':
            $upload[$name] = $edit[$name];
            unset($edit[$name]);
            break;
        }
      }
      if (!isset($post[$name]) && !$done) {
        switch ($type) {
          case 'textarea':
            $post[$name] = (string) $element;
            break;
          case 'select':
            $single = empty($element['multiple']);
            $first = TRUE;
            $index = 0;
            $key = preg_replace('/\[\]$/', '', $name);
            $options = $this->getAllOptions($element);
            foreach ($options as $option) {
              // For single select, we load the first option, if there is a
              // selected option that will overwrite it later.
              if ($option['selected'] || ($first && $single)) {
                $first = FALSE;
                if ($single) {
                  $post[$name] = (string) $option['value'];
                }
                else {
                  $post[$key . '[' . $index++ . ']'] = (string) $option['value'];
                }
              }
            }
            break;
          case 'file':
            break;
          case 'submit':
          case 'image':
            if (isset($submit) && $submit == $value) {
              $post[$name] = $value;
              $submit_matches = TRUE;
            }
            break;
          case 'radio':
          case 'checkbox':
            if (!isset($element['checked'])) {
              break;
            }
            $post[$name] = $value;
            break;
          default:
            $post[$name] = $value;
        }
      }
    }
    return $submit_matches;
  }

  /**
   * Builds an XPath query.
   *
   * Builds an XPath query by replacing placeholders in the query by the value
   * of the arguments.
   *
   * XPath 1.0 (the version supported by libxml2, the underlying XML library
   * used by PHP) doesn't support any form of quotation. This function
   * simplifies the building of XPath expression.
   *
   * @param $xpath
   *   An XPath query, possibly with placeholders in the form ':name'.
   * @param $args
   *   An array of arguments with keys in the form ':name' matching the
   *   placeholders in the query. The values may be either strings or numeric
   *   values.
   * @return
   *   An XPath query with arguments replaced.
   */
  protected function buildXPathQuery($xpath, array $args = array()) {
    // Replace placeholders.
    foreach ($args as $placeholder => $value) {
      // XPath 1.0 doesn't support a way to escape single or double quotes in a
      // string literal. We split double quotes out of the string, and encode
      // them separately.
      if (is_string($value)) {
        // Explode the text at the quote characters.
        $parts = explode('"', $value);

        // Quote the parts.
        foreach ($parts as &$part) {
          $part = '"' . $part . '"';
        }

        // Return the string.
        $value = count($parts) > 1 ? 'concat(' . implode(', \'"\', ', $parts) . ')' : $parts[0];
      }
      $xpath = preg_replace('/' . preg_quote($placeholder) . '\b/', $value, $xpath);
    }
    return $xpath;
  }

  /**
   * Perform an xpath search on the contents of the internal browser. The search
   * is relative to the root element (HTML tag normally) of the page.
   *
   * @param $xpath
   *   The xpath string to use in the search.
   *
   * @param array $arguments
   *   An array of arguments with keys in the form ':name' matching the
   *   placeholders in the query. The values may be either strings or numeric
   *   values.
   *
   * @return SimpleXmlElement[]|FALSE
   *   The return value of the xpath search. For details on the xpath string
   *   format and return values see the SimpleXML documentation,
   *   http://us.php.net/manual/function.simplexml-element-xpath.php.
   */
  protected function xpath($xpath, array $arguments = array()) {
    if ($this->parse()) {
      $xpath = $this->buildXPathQuery($xpath, $arguments);
      $result = $this->elements->xpath($xpath);
      // Some combinations of PHP / libxml versions return an empty array
      // instead of the documented FALSE. Forcefully convert any false-ish
      // values to an empty array to allow foreach(...) constructions.
      return $result ? $result : array();
    }
    else {
      return FALSE;
    }
  }

  /**
   * Get all option elements, including nested options, in a select.
   *
   * @param $element
   *   The element for which to get the options.
   * @return
   *   Option elements in select.
   */
  protected function getAllOptions(SimpleXMLElement $element) {
    $options = array();
    // Add all options items.
    foreach ($element->option as $option) {
      $options[] = $option;
    }

    // Search option group children.
    if (isset($element->optgroup)) {
      foreach ($element->optgroup as $group) {
        $options = array_merge($options, $this->getAllOptions($group));
      }
    }
    return $options;
  }

  /**
   * Pass if a link with the specified label is found, and optional with the
   * specified index.
   *
   * @param $label
   *   Text between the anchor tags.
   * @param $index
   *   Link position counting from zero.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to, defaults to 'Other'.
   * @return
   *   TRUE if the assertion succeeded, FALSE otherwise.
   */
  protected function assertLink($label, $index = 0, $message = '', $group = 'Other') {
    $links = $this->xpath('//a[normalize-space(text())=:label]', array(':label' => $label));
    $message = ($message ?  $message : t('Link with label %label found.', array('%label' => $label)));
    return $this->assert(isset($links[$index]), $message, $group);
  }

  /**
   * Pass if a link with the specified label is not found.
   *
   * @param $label
   *   Text between the anchor tags.
   * @param $index
   *   Link position counting from zero.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to, defaults to 'Other'.
   * @return
   *   TRUE if the assertion succeeded, FALSE otherwise.
   */
  protected function assertNoLink($label, $message = '', $group = 'Other') {
    $links = $this->xpath('//a[normalize-space(text())=:label]', array(':label' => $label));
    $message = ($message ?  $message : t('Link with label %label not found.', array('%label' => $label)));
    return $this->assert(empty($links), $message, $group);
  }

  /**
   * Pass if a link containing a given href (part) is found.
   *
   * @param $href
   *   The full or partial value of the 'href' attribute of the anchor tag.
   * @param $index
   *   Link position counting from zero.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to, defaults to 'Other'.
   *
   * @return
   *   TRUE if the assertion succeeded, FALSE otherwise.
   */
  protected function assertLinkByHref($href, $index = 0, $message = '', $group = 'Other') {
    $links = $this->xpath('//a[contains(@href, :href)]', array(':href' => $href));
    $message = ($message ?  $message : t('Link containing href %href found.', array('%href' => $href)));
    return $this->assert(isset($links[$index]), $message, $group);
  }

  /**
   * Pass if a link containing a given href (part) is not found.
   *
   * @param $href
   *   The full or partial value of the 'href' attribute of the anchor tag.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to, defaults to 'Other'.
   *
   * @return
   *   TRUE if the assertion succeeded, FALSE otherwise.
   */
  protected function assertNoLinkByHref($href, $message = '', $group = 'Other') {
    $links = $this->xpath('//a[contains(@href, :href)]', array(':href' => $href));
    $message = ($message ?  $message : t('No link containing href %href found.', array('%href' => $href)));
    return $this->assert(empty($links), $message, $group);
  }

  /**
   * Follows a link by name.
   *
   * Will click the first link found with this link text by default, or a later
   * one if an index is given. Match is case sensitive with normalized space.
   * The label is translated label.
   *
   * If the link is discovered and clicked, the test passes. Fail otherwise.
   *
   * @param $label
   *   Text between the anchor tags.
   * @param $index
   *   Link position counting from zero.
   * @return
   *   Page contents on success, or FALSE on failure.
   */
  protected function clickLink($label, $index = 0) {
    $url_before = $this->getUrl();
    $urls = $this->xpath('//a[normalize-space(text())=:label]', array(':label' => $label));
    if (isset($urls[$index])) {
      $url_target = $this->getAbsoluteUrl($urls[$index]['href']);
      $this->pass(t('Clicked link %label (@url_target) from @url_before', array('%label' => $label, '@url_target' => $url_target, '@url_before' => $url_before)), 'Browser');
      return $this->backdropGet($url_target);
    }
    $this->fail(t('Link %label does not exist on @url_before', array('%label' => $label, '@url_before' => $url_before)), 'Browser');
    return FALSE;
  }

  /**
   * Takes a path and returns an absolute path.
   *
   * @param $path
   *   A path from the internal browser content.
   * @return
   *   The $path with $base_url prepended, if necessary.
   */
  protected function getAbsoluteUrl($path) {
    global $base_url, $base_path;

    $parts = parse_url($path);
    if (empty($parts['host'])) {
      // Ensure that we have a string (and no xpath object).
      $path = (string) $path;
      // Strip $base_path, if existent.
      $length = strlen($base_path);
      if (substr($path, 0, $length) === $base_path) {
        $path = substr($path, $length);
      }
      // Ensure that we have an absolute path.
      if (strlen($path) && $path[0] !== '/') {
        $path = '/' . $path;
      }
      // Finally, prepend the $base_url.
      $path = $base_url . $path;
    }
    return $path;
  }

  /**
   * Get the current URL from the cURL handler.
   *
   * @return
   *   The current URL.
   */
  protected function getUrl() {
    return $this->url;
  }

  /**
   * Gets the HTTP response headers of the requested page. Normally we are only
   * interested in the headers returned by the last request. However, if a page
   * is redirected or HTTP authentication is in use, multiple requests will be
   * required to retrieve the page. Headers from all requests may be requested
   * by passing TRUE to this function.
   *
   * @param $all_requests
   *   Boolean value specifying whether to return headers from all requests
   *   instead of just the last request. Defaults to FALSE.
   * @return
   *   A name/value array if headers from only the last request are requested.
   *   If headers from all requests are requested, an array of name/value
   *   arrays, one for each request.
   *
   *   The pseudonym ":status" is used for the HTTP status line.
   *
   *   Values for duplicate headers are stored as a single comma-separated list.
   */
  protected function backdropGetHeaders($all_requests = FALSE) {
    $request = 0;
    $headers = array($request => array());
    foreach ($this->headers as $header) {
      $header = trim($header);
      if ($header === '') {
        $request++;
      }
      else {
        if (strpos($header, 'HTTP/') === 0) {
          $name = ':status';
          $value = $header;
        }
        else {
          list($name, $value) = explode(':', $header, 2);
          $name = strtolower($name);
        }
        if (isset($headers[$request][$name])) {
          $headers[$request][$name] .= ',' . trim($value);
        }
        else {
          $headers[$request][$name] = trim($value);
        }
      }
    }
    if (!$all_requests) {
      $headers = array_pop($headers);
    }
    return $headers;
  }

  /**
   * Gets the value of an HTTP response header. If multiple requests were
   * required to retrieve the page, only the headers from the last request will
   * be checked by default. However, if TRUE is passed as the second argument,
   * all requests will be processed from last to first until the header is
   * found.
   *
   * @param $name
   *   The name of the header to retrieve. Names are case-insensitive (see RFC
   *   2616 section 4.2).
   * @param $all_requests
   *   Boolean value specifying whether to check all requests if the header is
   *   not found in the last request. Defaults to FALSE.
   * @return string|FALSE
   *   The HTTP header value or FALSE if not found.
   */
  protected function backdropGetHeader($name, $all_requests = FALSE) {
    $name = strtolower($name);
    $header = FALSE;
    if ($all_requests) {
      foreach (array_reverse($this->backdropGetHeaders(TRUE)) as $headers) {
        if (isset($headers[$name])) {
          $header = $headers[$name];
          break;
        }
      }
    }
    else {
      $headers = $this->backdropGetHeaders();
      if (isset($headers[$name])) {
        $header = $headers[$name];
      }
    }
    return $header;
  }

  /**
   * Gets the current raw HTML of requested page.
   */
  protected function backdropGetContent() {
    return $this->content;
  }

  /**
   * Gets the value of the Backdrop.settings JavaScript variable for the currently loaded page.
   */
  protected function backdropGetSettings() {
    return $this->backdropSettings;
  }

  /**
   * Gets an array containing all emails sent during this test case.
   *
   * @param $filter
   *   An array containing key/value pairs used to filter the emails that are returned.
   * @return
   *   An array containing email messages captured during the current test.
   */
  protected function backdropGetMails($filter = array()) {
    $captured_emails = state_get('test_email_collector', array());
    $filtered_emails = array();

    foreach ($captured_emails as $message) {
      foreach ($filter as $key => $value) {
        if (!isset($message[$key]) || $message[$key] != $value) {
          continue 2;
        }
      }
      $filtered_emails[] = $message;
    }

    return $filtered_emails;
  }

  /**
   * Sets the raw HTML content. This can be useful when a page has been fetched
   * outside of the internal browser and assertions need to be made on the
   * returned page.
   *
   * A good example would be when testing backdrop_http_request(). After fetching
   * the page the content can be set and page elements can be checked to ensure
   * that the function worked properly.
   */
  protected function backdropSetContent($content, $url = 'internal:') {
    $this->content = $content;
    $this->url = $url;
    $this->plainTextContent = FALSE;
    $this->elements = FALSE;
    $this->backdropSettings = array();
    if (preg_match('/window.Backdrop[ ]?=[ ]?{settings:[ ]?(.*)}/', $content, $matches)) {
      $this->backdropSettings = backdrop_json_decode($matches[1]);
    }
  }

  /**
   * Sets the value of the Backdrop.settings JavaScript variable for the currently loaded page.
   */
  protected function backdropSetSettings($settings) {
    $this->backdropSettings = $settings;
  }

  /**
   * Pass if the internal browser's URL matches the given path.
   *
   * @param $path
   *   The expected system path.
   * @param $options
   *   (optional) Any additional options to pass for $path to url().
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to, defaults to 'Other'.
   *
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertUrl($path, array $options = array(), $message = '', $group = 'Other') {
    if (!$message) {
      $message = t('Current URL is @url.', array(
        '@url' => var_export(url($path, $options), TRUE),
      ));
    }
    $options['absolute'] = TRUE;
    return $this->assertEqual($this->getUrl(), url($path, $options), $message, $group);
  }

  /**
   * Pass if the raw text IS found on the loaded page, fail otherwise. Raw text
   * refers to the raw HTML that the page generated.
   *
   * @param $raw
   *   Raw (HTML) string to look for.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to, defaults to 'Other'.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertRaw($raw, $message = '', $group = 'Other') {
    if (!$message) {
      $message = t('Raw "@raw" found', array('@raw' => $raw));
    }
    return $this->assert(strpos($this->backdropGetContent(), (string) $raw) !== FALSE, $message, $group);
  }

  /**
   * Pass if the raw text is NOT found on the loaded page, fail otherwise. Raw text
   * refers to the raw HTML that the page generated.
   *
   * @param $raw
   *   Raw (HTML) string to look for.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to, defaults to 'Other'.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertNoRaw($raw, $message = '', $group = 'Other') {
    if (!$message) {
      $message = t('Raw "@raw" not found', array('@raw' => $raw));
    }
    return $this->assert(strpos($this->backdropGetContent(), (string) $raw) === FALSE, $message, $group);
  }

  /**
   * Pass if the text IS found on the text version of the page. The text version
   * is the equivalent of what a user would see when viewing through a web browser.
   * In other words the HTML has been filtered out of the contents.
   *
   * @param $text
   *   Plain text to look for.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to, defaults to 'Other'.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertText($text, $message = '', $group = 'Other') {
    return $this->assertTextHelper($text, $message, $group, FALSE);
  }

  /**
   * Pass if the text is NOT found on the text version of the page. The text version
   * is the equivalent of what a user would see when viewing through a web browser.
   * In other words the HTML has been filtered out of the contents.
   *
   * @param $text
   *   Plain text to look for.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to, defaults to 'Other'.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertNoText($text, $message = '', $group = 'Other') {
    return $this->assertTextHelper($text, $message, $group, TRUE);
  }

  /**
   * Helper for assertText and assertNoText.
   *
   * It is not recommended to call this function directly.
   *
   * @param $text
   *   Plain text to look for.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to.
   * @param $not_exists
   *   TRUE if this text should not exist, FALSE if it should.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertTextHelper($text, $message, $group, $not_exists) {
    if ($this->plainTextContent === FALSE) {
      $this->plainTextContent = filter_xss($this->backdropGetContent(), array());
    }
    if (!$message) {
      $message = !$not_exists ? t('"@text" found', array('@text' => $text)) : t('"@text" not found', array('@text' => $text));
    }
    return $this->assert($not_exists == (strpos($this->plainTextContent, $text) === FALSE), $message, $group);
  }

  /**
   * Pass if the text is found ONLY ONCE on the text version of the page.
   *
   * The text version is the equivalent of what a user would see when viewing
   * through a web browser. In other words the HTML has been filtered out of
   * the contents.
   *
   * @param $text
   *   Plain text to look for.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to, defaults to 'Other'.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertUniqueText($text, $message = '', $group = 'Other') {
    return $this->assertUniqueTextHelper($text, $message, $group, TRUE);
  }

  /**
   * Pass if the text is found MORE THAN ONCE on the text version of the page.
   *
   * The text version is the equivalent of what a user would see when viewing
   * through a web browser. In other words the HTML has been filtered out of
   * the contents.
   *
   * @param $text
   *   Plain text to look for.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to, defaults to 'Other'.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertNoUniqueText($text, $message = '', $group = 'Other') {
    return $this->assertUniqueTextHelper($text, $message, $group, FALSE);
  }

  /**
   * Helper for assertUniqueText and assertNoUniqueText.
   *
   * It is not recommended to call this function directly.
   *
   * @param $text
   *   Plain text to look for.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to.
   * @param $be_unique
   *   TRUE if this text should be found only once, FALSE if it should be found more than once.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertUniqueTextHelper($text, $message, $group, $be_unique) {
    if ($this->plainTextContent === FALSE) {
      $this->plainTextContent = filter_xss($this->backdropGetContent(), array());
    }
    if (!$message) {
      $message = '"' . $text . '"' . ($be_unique ? ' found only once' : ' found more than once');
    }
    $first_occurrence = strpos($this->plainTextContent, $text);
    if ($first_occurrence === FALSE) {
      return $this->assert(FALSE, $message, $group);
    }
    $offset = $first_occurrence + strlen($text);
    $second_occurrence = strpos($this->plainTextContent, $text, $offset);
    return $this->assert($be_unique == ($second_occurrence === FALSE), $message, $group);
  }

  /**
   * Will trigger a pass if the Perl regex pattern is found in the raw content.
   *
   * @param $pattern
   *   Perl regex to look for including the regex delimiters.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertPattern($pattern, $message = '', $group = 'Other') {
    if (!$message) {
      $message = t('Pattern "@pattern" found', array('@pattern' => $pattern));
    }
    return $this->assert((bool) preg_match($pattern, $this->backdropGetContent()), $message, $group);
  }

  /**
   * Will trigger a pass if the perl regex pattern is not present in raw content.
   *
   * @param $pattern
   *   Perl regex to look for including the regex delimiters.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertNoPattern($pattern, $message = '', $group = 'Other') {
    if (!$message) {
      $message = t('Pattern "@pattern" not found', array('@pattern' => $pattern));
    }
    return $this->assert(!preg_match($pattern, $this->backdropGetContent()), $message, $group);
  }

  /**
   * Pass if the page title is the given string.
   *
   * @param $title
   *   The string the title should be.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertTitle($title, $message = '', $group = 'Other') {
    $actual = (string) current($this->xpath('//title'));
    if (!$message) {
      $message = t('Page title @actual is equal to @expected.', array(
        '@actual' => var_export($actual, TRUE),
        '@expected' => var_export($title, TRUE),
      ));
    }
    return $this->assertEqual($actual, $title, $message, $group);
  }

  /**
   * Pass if the page title is not the given string.
   *
   * @param $title
   *   The string the title should not be.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertNoTitle($title, $message = '', $group = 'Other') {
    $actual = (string) current($this->xpath('//title'));
    if (!$message) {
      $message = t('Page title @actual is not equal to @unexpected.', array(
        '@actual' => var_export($actual, TRUE),
        '@unexpected' => var_export($title, TRUE),
      ));
    }
    return $this->assertNotEqual($actual, $title, $message, $group);
  }

  /**
   * Asserts themed output.
   *
   * @param $callback
   *   The name of the theme function to invoke; e.g. 'links' for theme_links().
   * @param $variables
   *   An array of variables to pass to the theme function.
   * @param $expected
   *   The expected themed output string.
   * @param $message
   *   (optional) A message to display with the assertion. Do not translate
   *   messages: use format_string() to embed variables in the message text, not
   *   t(). If left blank, a default message will be displayed.
   * @param $group
   *   (optional) The group this message is in, which is displayed in a column
   *   in test output. Use 'Debug' to indicate this is debugging output. Do not
   *   translate this string. Defaults to 'Other'; most tests do not override
   *   this default.
   *
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertThemeOutput($callback, array $variables, $expected, $message = '', $group = 'Other') {
    $output = theme($callback, $variables);
    $this->verbose('Variables:<pre>' .  check_plain(var_export($variables, TRUE)) . '</pre>'
      . '<hr />Result:<pre>' .  check_plain(var_export($output, TRUE)) . '</pre>'
      . '<hr />Expected:<pre>' .  check_plain(var_export($expected, TRUE)) . '</pre>'
      . '<hr />' . $output
    );
    if (!$message) {
      $message = '%callback rendered correctly.';
    }
    $message = format_string($message, array('%callback' => 'theme_' . $callback . '()'));
    return $this->assertIdentical($output, $expected, $message, $group);
  }

  /**
   * Asserts that a field exists in the current page by the given XPath.
   *
   * @param $xpath
   *   XPath used to find the field.
   * @param $value
   *   (optional) Value of the field to assert.
   * @param $message
   *   (optional) Message to display.
   * @param $group
   *   (optional) The group this message belongs to.
   *
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertFieldByXPath($xpath, $value = NULL, $message = '', $group = 'Other') {
    $fields = $this->xpath($xpath);

    // If value specified then check array for match.
    $found = TRUE;
    if (isset($value)) {
      $found = FALSE;
      if ($fields) {
        foreach ($fields as $field) {
          if (isset($field['value']) && $field['value'] == $value) {
            // Input element with correct value.
            $found = TRUE;
          }
          elseif (isset($field->option)) {
            // Select element found.
            if ($this->getSelectedItem($field) == $value) {
              $found = TRUE;
            }
            else {
              // No item selected so use first item.
              $items = $this->getAllOptions($field);
              if (!empty($items) && $items[0]['value'] == $value) {
                $found = TRUE;
              }
            }
          }
          elseif ((string) $field == $value) {
            // Text area with correct text.
            $found = TRUE;
          }
        }
      }
    }
    return $this->assertTrue($fields && $found, $message, $group);
  }

  /**
   * Get the selected value from a select field.
   *
   * @param $element
   *   SimpleXMLElement select element.
   * @return
   *   The selected value or FALSE.
   */
  protected function getSelectedItem(SimpleXMLElement $element) {
    foreach ($element->children() as $item) {
      if (isset($item['selected'])) {
        return $item['value'];
      }
      elseif ($item->getName() == 'optgroup') {
        if ($value = $this->getSelectedItem($item)) {
          return $value;
        }
      }
    }
    return FALSE;
  }

  /**
   * Asserts that a field does not exist in the current page by the given XPath.
   *
   * @param $xpath
   *   XPath used to find the field.
   * @param $value
   *   (optional) Value of the field to assert.
   * @param $message
   *   (optional) Message to display.
   * @param $group
   *   (optional) The group this message belongs to.
   *
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertNoFieldByXPath($xpath, $value = NULL, $message = '', $group = 'Other') {
    $fields = $this->xpath($xpath);

    // If value specified then check array for match.
    $found = TRUE;
    if (isset($value)) {
      $found = FALSE;
      if ($fields) {
        foreach ($fields as $field) {
          if ($field['value'] == $value) {
            $found = TRUE;
          }
        }
      }
    }
    return $this->assertFalse($fields && $found, $message, $group);
  }

  /**
   * Asserts that a field exists in the current page with the given name and value.
   *
   * @param $name
   *   Name of field to assert.
   * @param $value
   *   Value of the field to assert.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertFieldByName($name, $value = NULL, $message = NULL) {
    if (!isset($message)) {
      if (!isset($value)) {
        $message = t('Found field with name @name', array(
          '@name' => var_export($name, TRUE),
        ));
      }
      else {
        $message = t('Found field with name @name and value @value', array(
          '@name' => var_export($name, TRUE),
          '@value' => var_export($value, TRUE),
        ));
      }
    }
    return $this->assertFieldByXPath($this->constructFieldXpath('name', $name), $value, $message, t('Browser'));
  }

  /**
   * Asserts that a field does not exist with the given name and value.
   *
   * @param $name
   *   Name of field to assert.
   * @param $value
   *   Value of the field to assert.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertNoFieldByName($name, $value = '', $message = '') {
    return $this->assertNoFieldByXPath($this->constructFieldXpath('name', $name), $value, $message ? $message : t('Did not find field by name @name', array('@name' => $name)), t('Browser'));
  }

  /**
   * Asserts that a field exists in the current page with the given id and value.
   *
   * @param $id
   *   Id of field to assert.
   * @param $value
   *   Value of the field to assert.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertFieldById($id, $value = '', $message = '') {
    return $this->assertFieldByXPath($this->constructFieldXpath('id', $id), $value, $message ? $message : t('Found field by id @id', array('@id' => $id)), t('Browser'));
  }

  /**
   * Asserts that a field does not exist with the given id and value.
   *
   * @param $id
   *   Id of field to assert.
   * @param $value
   *   Value of the field to assert.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertNoFieldById($id, $value = '', $message = '') {
    return $this->assertNoFieldByXPath($this->constructFieldXpath('id', $id), $value, $message ? $message : t('Did not find field by id @id', array('@id' => $id)), t('Browser'));
  }

  /**
   * Asserts that a checkbox field in the current page is checked.
   *
   * @param $id
   *   Id of field to assert.
   * @param $message
   *   Message to display.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertFieldChecked($id, $message = '') {
    $elements = $this->xpath('//input[@id=:id]', array(':id' => $id));
    return $this->assertTrue(isset($elements[0]) && !empty($elements[0]['checked']), $message ? $message : t('Checkbox field @id is checked.', array('@id' => $id)), t('Browser'));
  }

  /**
   * Asserts that a checkbox field in the current page is not checked.
   *
   * @param $id
   *   Id of field to assert.
   * @param $message
   *   Message to display.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertNoFieldChecked($id, $message = '') {
    $elements = $this->xpath('//input[@id=:id]', array(':id' => $id));
    return $this->assertTrue(isset($elements[0]) && empty($elements[0]['checked']), $message ? $message : t('Checkbox field @id is not checked.', array('@id' => $id)), t('Browser'));
  }

  /**
   * Asserts that a select option in the current page exists.
   *
   * @param $id
   *   Id of select field to assert.
   * @param $option
   *   Option to assert.
   * @param $message
   *   Message to display.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertOption($id, $option, $message = '') {
    $options = $this->xpath('//select[@id=:id]//option[@value=:option]', array(':id' => $id, ':option' => $option));
    return $this->assertTrue(isset($options[0]), $message ? $message : t('Option @option for field @id exists.', array('@option' => $option, '@id' => $id)), t('Browser'));
  }

  /**
   * Asserts that a select option in the current page does not exist.
   *
   * @param $id
   *   Id of select field to assert.
   * @param $option
   *   Option to assert.
   * @param $message
   *   Message to display.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertNoOption($id, $option, $message = '') {
    $selects = $this->xpath('//select[@id=:id]', array(':id' => $id));
    $options = $this->xpath('//select[@id=:id]/option[@value=:option]', array(':id' => $id, ':option' => $option));
    return $this->assertTrue(isset($selects[0]) && !isset($options[0]), $message ? $message : t('Option @option for field @id does not exist.', array('@option' => $option, '@id' => $id)), t('Browser'));
  }

  /**
   * Asserts that a select option in the current page is checked.
   *
   * @param $id
   *   Id of select field to assert.
   * @param $option
   *   Option to assert.
   * @param $message
   *   Message to display.
   * @return
   *   TRUE on pass, FALSE on fail.
   *
   * @todo $id is unusable. Replace with $name.
   */
  protected function assertOptionSelected($id, $option, $message = '') {
    $elements = $this->xpath('//select[@id=:id]//option[@value=:option]', array(':id' => $id, ':option' => $option));
    return $this->assertTrue(isset($elements[0]) && !empty($elements[0]['selected']), $message ? $message : t('Option @option for field @id is selected.', array('@option' => $option, '@id' => $id)), t('Browser'));
  }

  /**
   * Asserts that a select option in the current page is not checked.
   *
   * @param $id
   *   Id of select field to assert.
   * @param $option
   *   Option to assert.
   * @param $message
   *   Message to display.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertNoOptionSelected($id, $option, $message = '') {
    $elements = $this->xpath('//select[@id=:id]//option[@value=:option]', array(':id' => $id, ':option' => $option));
    return $this->assertTrue(isset($elements[0]) && empty($elements[0]['selected']), $message ? $message : t('Option @option for field @id is not selected.', array('@option' => $option, '@id' => $id)), t('Browser'));
  }

  /**
   * Asserts that a field exists with the given name or id.
   *
   * @param $field
   *   Name or id of field to assert.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertField($field, $message = '', $group = 'Other') {
    return $this->assertFieldByXPath($this->constructFieldXpath('name', $field) . '|' . $this->constructFieldXpath('id', $field), NULL, $message, $group);
  }

  /**
   * Asserts that a field does not exist with the given name or id.
   *
   * @param $field
   *   Name or id of field to assert.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertNoField($field, $message = '', $group = 'Other') {
    return $this->assertNoFieldByXPath($this->constructFieldXpath('name', $field) . '|' . $this->constructFieldXpath('id', $field), NULL, $message, $group);
  }

  /**
   * Asserts that each HTML ID is used for just a single element.
   *
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to.
   * @param $ids_to_skip
   *   An optional array of ids to skip when checking for duplicates. It is
   *   always a bug to have duplicate HTML IDs, so this parameter is to enable
   *   incremental fixing of core code. Whenever a test passes this parameter,
   *   it should add a "todo" comment above the call to this function explaining
   *   the legacy bug that the test wishes to ignore and including a link to an
   *   issue that is working to fix that legacy bug.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertNoDuplicateIds($message = '', $group = 'Other', $ids_to_skip = array()) {
    $status = TRUE;
    foreach ($this->xpath('//*[@id]') as $element) {
      $id = (string) $element['id'];
      if (isset($seen_ids[$id]) && !in_array($id, $ids_to_skip)) {
        $this->fail(t('The HTML ID %id is unique.', array('%id' => $id)), $group);
        $status = FALSE;
      }
      $seen_ids[$id] = TRUE;
    }
    return $this->assert($status, $message, $group);
  }

  /**
   * Helper function: construct an XPath for the given set of attributes and value.
   *
   * @param $attribute
   *   Field attributes.
   * @param $value
   *   Value of field.
   * @return
   *   XPath for specified values.
   */
  protected function constructFieldXpath($attribute, $value) {
    $xpath = '//textarea[@' . $attribute . '=:value]|//input[@' . $attribute . '=:value]|//select[@' . $attribute . '=:value]';
    return $this->buildXPathQuery($xpath, array(':value' => $value));
  }

  /**
   * Asserts the page responds with the specified response code.
   *
   * @param $code
   *   Response code. For example 200 is a successful page request. For a list
   *   of all codes see http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html.
   * @param $message
   *   Message to display.
   * @return
   *   Assertion result.
   */
  protected function assertResponse($code, $message = '') {
    $curl_code = curl_getinfo($this->curlHandle, CURLINFO_HTTP_CODE);
    $match = is_array($code) ? in_array($curl_code, $code) : $curl_code == $code;
    return $this->assertTrue($match, $message ? $message : t('HTTP response expected !code, actual !curl_code', array('!code' => $code, '!curl_code' => $curl_code)), t('Browser'));
  }

  /**
   * Asserts the page did not return the specified response code.
   *
   * @param $code
   *   Response code. For example 200 is a successful page request. For a list
   *   of all codes see http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html.
   * @param $message
   *   Message to display.
   *
   * @return
   *   Assertion result.
   */
  protected function assertNoResponse($code, $message = '') {
    $curl_code = curl_getinfo($this->curlHandle, CURLINFO_HTTP_CODE);
    $match = is_array($code) ? in_array($curl_code, $code) : $curl_code == $code;
    return $this->assertFalse($match, $message ? $message : t('HTTP response not expected !code, actual !curl_code', array('!code' => $code, '!curl_code' => $curl_code)), t('Browser'));
  }

  /**
   * Asserts that the most recently sent email message has the given value.
   *
   * The field in $name must have the content described in $value.
   *
   * @param $name
   *   Name of field or message property to assert. Examples: subject, body, id, ...
   * @param $value
   *   Value of the field to assert.
   * @param $message
   *   Message to display.
   *
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertMail($name, $value = '', $message = '') {
    $captured_emails = state_get('test_email_collector', array());
    $email = end($captured_emails);
    return $this->assertTrue($email && isset($email[$name]) && $email[$name] == $value, $message, t('Email'));
  }

  /**
   * Asserts that the most recently sent email message has the string in it.
   *
   * @param $field_name
   *   Name of field or message property to assert: subject, body, id, ...
   * @param $string
   *   String to search for.
   * @param $email_depth
   *   Number of emails to search for string, starting with most recent.
   *
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertMailString($field_name, $string, $email_depth) {
    $mails = $this->backdropGetMails();
    $string_found = FALSE;
    for ($i = sizeof($mails) -1; $i >= sizeof($mails) - $email_depth && $i >= 0; $i--) {
      $mail = $mails[$i];
      // Normalize whitespace, as we don't know what the mail system might have
      // done. Any run of whitespace becomes a single space.
      $normalized_mail = preg_replace('/\s+/', ' ', $mail[$field_name]);
      $normalized_string = preg_replace('/\s+/', ' ', $string);
      $string_found = (FALSE !== strpos($normalized_mail, $normalized_string));
      if ($string_found) {
        break;
      }
    }
    return $this->assertTrue($string_found, t('Expected text found in @field of email message: "@expected".', array('@field' => $field_name, '@expected' => $string)));
  }

  /**
   * Asserts that the most recently sent email message has the pattern in it.
   *
   * @param $field_name
   *   Name of field or message property to assert: subject, body, id, ...
   * @param $regex
   *   Pattern to search for.
   *
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertMailPattern($field_name, $regex, $message) {
    $mails = $this->backdropGetMails();
    $mail = end($mails);
    $regex_found = preg_match("/$regex/", $mail[$field_name]);
    return $this->assertTrue($regex_found, t('Expected text found in @field of email message: "@expected".', array('@field' => $field_name, '@expected' => $regex)));
  }

  /**
   * Outputs to verbose the most recent $count emails sent.
   *
   * @param $count
   *   Optional number of emails to output.
   */
  protected function verboseEmail($count = 1) {
    $mails = $this->backdropGetMails();
    for ($i = sizeof($mails) -1; $i >= sizeof($mails) - $count && $i >= 0; $i--) {
      $mail = $mails[$i];
      $this->verbose(t('Email:') . '<pre>' . print_r($mail, TRUE) . '</pre>');
    }
  }

  /**
   * Verifies that a watchdog message has been entered.
   *
   * @param string $watchdog_message
   *   The watchdog message.
   * @param array $variables
   *   The array of variables passed to watchdog().
   * @param string $message
   *   The assertion message.
   *
   * @since 1.19.0 Method added.
   */
  protected function assertWatchdogMessage($watchdog_message, array $variables, $message) {
    $status = (bool) db_query_range("SELECT 1 FROM {watchdog} WHERE message = :message AND variables = :variables", 0, 1, array(
      ':message' => $watchdog_message,
      ':variables' => serialize($variables),
    ))->fetchField();
    return $this->assert($status, format_string('@message', array('@message' => $message)));
  }

  /**
   * Clears the watchdog database table.
   *
   * @since 1.19.0 Method added.
   */
  protected function clearWatchdog() {
    db_truncate('watchdog')->execute();
  }
}

/**
 * Logs verbose message in a text file.
 *
 * If verbose mode is enabled then page requests will be dumped to a file and
 * presented on the test result screen. The messages will be placed in a file
 * located in the simpletest directory in the original file system.
 *
 * @param $message
 *   The verbose message to be stored.
 * @param $original_file_directory
 *   The original file directory, before it was changed for testing purposes.
 * @param $test_class
 *   The active test case class.
 *
 * @return
 *   The ID of the message to be placed in related assertion messages.
 *
 * @see BackdropTestCase->originalFileDirectory
 * @see BackdropWebTestCase->verbose()
 */
function simpletest_verbose($message, $original_file_directory = NULL, $test_class = NULL) {
  static $file_directory = NULL, $class = NULL, $id = 1, $verbose = NULL;

  // Will pass first time during setup phase, and when verbose is TRUE.
  if (!isset($original_file_directory) && !$verbose) {
    return FALSE;
  }

  if ($message && $file_directory) {
    $message = '<hr />ID #' . $id . ' (<a href="' . $class . '-' . ($id - 1) . '.html">Previous</a> | <a href="' . $class . '-' . ($id + 1) . '.html">Next</a>)<hr />' . $message;
    file_put_contents($file_directory . "/simpletest/verbose/$class-$id.html", $message, FILE_APPEND);
    return $id++;
  }

  if ($original_file_directory) {
    $file_directory = $original_file_directory;
    $class = $test_class;
    $verbose = config_get('simpletest.settings', 'simpletest_verbose');
    $directory = $file_directory . '/simpletest/verbose';
    $writable = file_prepare_directory($directory, FILE_CREATE_DIRECTORY);
    if ($writable && !file_exists($directory . '/.htaccess')) {
      file_put_contents($directory . '/.htaccess', "<IfModule mod_expires.c>\nExpiresActive Off\n</IfModule>\n");
    }
    return $writable;
  }
  return FALSE;
}
