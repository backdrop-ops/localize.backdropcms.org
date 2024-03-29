<?php

/**
 * Implements hook_views_data_alter().
 */
function views_bulk_operations_views_data_alter(&$data) {
  foreach (entity_get_info() as $entity_type => $info) {
    if (isset($info['base table']) && isset($data[$info['base table']]['table'])) {
      $data[$info['base table']]['views_bulk_operations']['moved to'] = array('views_entity_' . $entity_type, 'views_bulk_operations');
      $data['views_entity_' . $entity_type]['views_bulk_operations'] = array(
        'title' => $data[$info['base table']]['table']['group'],
        'group' => t('Views bulk operations'),
        'help' => t('Provide a checkbox to select the row for bulk operations.'),
        'real field' => $info['entity keys']['id'],
        'field' => array(
          'handler' => 'ViewsBulkOperationsHandlerFieldOperations',
          'click sortable' => FALSE,
        ),
      );
      // Check that the base table has the entity type key set.
      if (!isset($data[$info['base table']]['table']['entity type'])) {
        $data[$info['base table']]['table']['entity type'] = $entity_type;
      }
    }
    if (isset($info['revision table']) && isset($data[$info['revision table']]['table'])) {
      $data[$info['revision table']]['views_bulk_operations'] = array(
        'title' => $data[$info['revision table']]['table']['group'],
        'group' => t('Views bulk operations'),
        'help' => t('Provide a checkbox to select the row for bulk operations.'),
        'real field' => $info['entity keys']['revision'],
        'field' => array(
          'handler' => 'ViewsBulkOperationsHandlerFieldOperations',
          'click sortable' => FALSE,
        ),
      );
    }
  }
}
