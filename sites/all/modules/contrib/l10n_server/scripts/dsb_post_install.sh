#!/bin/sh

# Get the token in to get the key.
cd ${BUILD_TARGET}/sites/client
token=`drush eval '$user = user_load(1); echo l10n_client_user_token($user);'`
echo "URL token to access server API key is $token"

# Fetch the API key for the user on the server.
cd ${BUILD_TARGET}/sites/server
key=`drush eval "echo l10n_remote_user_api_key(1, '$token');"`
echo "API key for admin user is $key"

# Set the API key of the user on the client in order to connect to the server.
cd ${BUILD_TARGET}/sites/client
drush eval "user_save(user_load(1), array('data' => array('l10n_client_key' => '$key')));"
echo "API key was saved in the user profile on the client"

# Send some translation suggestions from the client to the server.
drush eval "l10n_client_submit_translation('de', 'Title', 'German translation suggestion for Title', '$key', '$token');"
echo "Sent translation suggestion for 'Title' string of simplenews project from client to server using XMLRPC"

drush eval "l10n_client_submit_translation('de', 'user', 'German translation suggestion for user', '$key', '$token');"
echo "Sent translation suggestion for 'user' string of simplenews project from client to server using XMLRPC"

drush eval "l10n_client_submit_translation('de', 'Status', 'German translation suggestion for Status', '$key', '$token');"
echo "Sent translation suggestion for 'Status' string of simplenews project from client to server using XMLRPC"
