<?php
/**
 * @file
 * Defines the bee commands for the Rules module.
 */

/**
 * Implements hook_bee_command().
 */
function rules_bee_command() {

  $items['rules-enable-rule'] = array(
    'description' => "Enable a Rule.",
    'callback' => 'bee_rules_enable_rule',
    'group' => 'content',
    'arguments' => array(
      'rule_name' => bt('The machine name of the rule to be enabled.'),
    ),
    'aliases' => array('rer'),
    'bootstrap' => BEE_BOOTSTRAP_FULL,
    'examples' => array(
      'bee rer rule_test_rule' => bt('Enable the rule with machine name rule_test_rule.'),
    ),
  );

  $items['rules-disable-rule'] = array(
    'description' => "Disable a Rule.",
    'callback' => 'bee_rules_disable_rule',
    'group' => 'content',
    'arguments' => array(
      'rule_name' => bt('The machine name of the rule to be disabled.'),
    ),
    'aliases' => array('rdr'),
    'bootstrap' => BEE_BOOTSTRAP_FULL,
    'examples' => array(
      'bee rdr rule_test_rule' => bt('Disable the rule with machine name rule_test_rule.'),
    ),
  );

  return $items;
}

/**
 * Enable a Rule.
 */
function bee_rules_enable_rule($arguments, $options) {
  $rule_name = $arguments['rule_name'];
  $rule_config = rules_config_load($rule_name);
  if (empty($rule_config)) {
    bee_message(bt('There is no Rule with machine name @rule_name.', array('@rule_name' => $rule_name)), 'error');
    return;
  }
  if ($rule_config->active) {
    bee_message(bt('Rule @rule_name is already enabled.', array('@rule_name' => $rule_name)), 'warning');
    return;
  }
  $rule_config->active = TRUE;
  $rule_config->save();
  bee_message(bt('Rule @rule_name has been enabled.', array('@rule_name' => $rule_name)), 'success');
}

/**
 * Disable a Rule.
 */
function bee_rules_disable_rule($arguments, $options) {
  $rule_name = $arguments['rule_name'];
  $rule_config = rules_config_load($rule_name);
  if (empty($rule_config)) {
    bee_message(bt('There is no Rule with machine name @rule_name.', array('@rule_name' => $rule_name)), 'error');
    return;
  }
  if (!$rule_config->active) {
    bee_message(bt('Rule @rule_name is already disabled.', array('@rule_name' => $rule_name)), 'warning');
    return;
  }
  $rule_config->active = FALSE;
  $rule_config->save();
  bee_message(bt('Rule @rule_name has been disabled.', array('@rule_name' => $rule_name)), 'success');
}
