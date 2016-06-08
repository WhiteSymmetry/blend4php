<?php
require_once 'Error.inc';

/**
 * @file
 *
 * HTTPRequest
 *
 * The HTTPRequest Class contains methods for using CURL rest options.
 * More specifically, UPDATE, POST, DELETE, PUT and GET.
 *
 */

class GalaxyHTTPRequest  {

   // Global Variable that contains any errors received from a rest request.
   private $error = NULL;

   // A list of cookies from the most recent request.
   private $cookies = array();

   /**
    * The GalaxyHTTPRequest constructor.
    *
    * @param RequestError requesterror
    *   Optional, set the class's request error to a pre-existing RequestError.
    *
    * @return
    *   An instance of a RestManager class.
    *
    *
    */
   function __construct() {
     $this->error = new GalaxyError();
   }
   /**
    * Retreives the most recent cookies from the lastest request.
    *
    * @return
    *   An array of cookies where the key is the cookie name and the value
    *   is the cookie string.
    */
   public function getCookies() {
      return $this->cookies;
   }

  /**
   * Set the error for this class by extracting from the given CURL response.
   *
   * @param $response
   *   curl response from galaxy page $message.
   *
   * @return
   *   TRUE if error was found, FALSE otherwise.
   *
   *
   */
  private function getCURLResponse($ch) {

    // Make sure we get the header included with the output.
    curl_setopt($ch, CURLOPT_HEADER, 1);
    $output = curl_exec($ch);
    if ($output === FALSE) {
      $this->error->setError('HTTP', curl_error($ch));
      return FALSE;
    }

    // Separate the header from the body.
    $header_size = curl_getinfo($ch, CURLINFO_HEADER_SIZE);
    $temp = substr($output, 0, $header_size);
    $temp = explode("\n", $temp);
    $headers = array();
    foreach ($temp as $header) {
      $matches = array();
      if(preg_match('/^(.+?):\s*(.+)$/', $header, $matches)) {
        if (array_key_exists(strtolower($matches[1]), $headers)) {
          $headers[strtolower($matches[1])] = array();
        }
        $headers[strtolower($matches[1])][] = strtolower($matches[2]);
      }
    }
    // Get any cookies and save them
    if (array_key_exists('set-cookie', $headers)) {
      foreach ($headers['set-cookie'] as $i => $cookie) {
        // Split cookie
        $matches = array();
        preg_match('/^(.+?)=(.+?);.*$/', $cookie, $matches);
        if ($matches[1] == 'galaxysession') {
          $this->cookies[$matches[1]] = $matches[2];
        }
      }
    }

    $body = substr($output, $header_size);
    if(count($headers) == 0){
      $this->error->setError('Galaxy', "Galaxy did not return headers: \n" . print_r($output, TRUE));
      return FALSE;
    }

    // Check the header to see how to deal with the content.  Normally will
    // alawys have JSON but in case not we should handle it gracefully.
    $content_type = $headers['content-type'][0];
    if (preg_match('/x-gzip/', $content_type)) {
      return $body;
    }
    // Automatically convert JSON format to an array and deal with any errors.
    else if(preg_match('/application\/json/', $content_type)) {

      // Case 1: Galaxy just returns 'null'.
      if ($body == "null") {
        $this->error->setError('Galaxy', 'Unknown response from Galaxy: null');
        return FALSE;
      }

      // All other cases we can decode the JSON.
      $response = json_decode($body, TRUE);

      // Case 2:  The message is not in JSON format.
      if ($response === NULL) {
        $this->error->setError('Galaxy', 'Unknown CURL response: not json compatible: "' . $output . '".');
        return FALSE;
      }

      // Case 3:  The response is just a string
      if (!is_array($response)) {
        // If it's an empty string then something must be wrong.
        // return an error.
        if (!$response) {
          $this->error->setError('Galaxy', 'No value returned by Galaxy.');
          return FALSE;
        }
        return $response;
      }

      // Case 4: If we have a traceback element, we know the error originated
      // from python, meaning the request successfully made it to python but
      // there is ap problemo in the Galaxy code.
      if (array_key_exists('traceback', $response)) {
        // Case 4a: The traceback had an error message.
        if (array_key_exists('err_msg', $response)) {
          $this->error->setError('Galaxy', $response['err_msg']);
          return FALSE;
        }
        // Case 4b: The traceback did not have an error.
        else {

          $this->error->setError('Galaxy', 'A galaxy error was detected, though its contents are unknown');
          return FALSE;
        }
      }
      // Case 5: Galaxy generated the error.
      else if (array_key_exists('error', $response)) {
        $this->error->setError('Galaxy', $response['error']);
        return FALSE;
      }
      // Case 6: Galaxy generated the error but the message has two separate
      // components
      else if (array_key_exists('err_msg', $response)) {
        $this->error->setError('Galaxy', $response['err_msg']);
        return FALSE;
      }

      // Case 7:  No error found.
      return $response;
    }
    $this->error->setError('HTTP', 'Unsupported content-type: ' . trim($content_type) . "\nContent:\n" . print_r($output, TRUE));
    return FALSE;
  }

  /**
   * Performs a GET request.
   *
   * @param $URL
   *   The URL to perform the GET request on.
   * @param $curl_options
   *   Any additional information to retreive from the resource while
   *   the GET request...yo.  
   * @return
   *   curl server response.
   *
   *
   */
  public function httpGET($URL, $curl_options = array()) {
    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, $URL);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, TRUE);
    foreach ($curl_options as $option => $value) {
      curl_setopt($ch, $option, $value);
    }
    $response = $this->getCURLResponse($ch);
    curl_close($ch);

    return $response;
  }

  /**
   * Perform a POST request.
   *
   * @param URL
   *   The url to perform the POST request on.
   * @param Input
   *   The input data to a given URL.
   *
   * @return
   *   curl server response.
   *
   *
   */
  public function httpPOST($URL, $post_fields = NULL) {

    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, $URL);
    curl_setopt($ch, CURLOPT_POST, 1);
    curl_setopt($ch, CURLOPT_SAFE_UPLOAD, 1);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, True);
    if ($post_fields !== NULL) {
      curl_setopt($ch, CURLOPT_POSTFIELDS, $post_fields);
    }

    $response = $this->getCURLResponse($ch);

    return $response;
  }

  /**
   * Perform a PUT request
   *
   * @param URL
   *   The URL to perform the PUT request on.
   * @param Input
   *   The input data to give to the URL.
   *
   * @return
   *   curl server response.
   *
   *
   */
  public function httpPUT($URL, $input = NULL) {
    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, $URL);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "PUT");
    if ($input != NULL) {
      curl_setopt($ch, CURLOPT_POSTFIELDS, $input);
    }
    $response = $this->getCURLResponse($ch);
    curl_close($ch);

    return $response;
  }

  /**
   * Perform a PATCH request
   *
   * @param URL
   *   The URL to perform the PUT request on.
   * @param Input
   *   The input data to give to the URL.
   *
   * @return
   *   curl server response.
   *
   *
   */
  public function httpPATCH($URL, $input = NULL) {
    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, $URL);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "PATCH");
    if ($input != NULL) {
      curl_setopt($ch, CURLOPT_POSTFIELDS, $input);
    }
    $response = $this->getCURLResponse($ch);
    curl_close($ch);

    return $response;
  }

  /**
   *
   * @param $URL
   * @param $file_name
   * @return
   */
  public function getRemoteFile($URL, $file_name) {

    set_time_limit(0);
    $fp = fopen($file_name, 'w+');
    if (!$fp) {
      $this->error->setError('FILE', "Cannot save file: " . $file_name);
      return FALSE;
    }
    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, $URL);
    curl_setopt($ch, CURLOPT_TIMEOUT, 50);
    curl_setopt($ch, CURLOPT_FILE, $fp);
    curl_setopt($ch, CURLOPT_FOLLOWLOCATION, TRUE);
    $output = curl_exec($ch);

    if ($output === FALSE) {
      $this->error->setError('HTTP', curl_error($ch));
      return FALSE;
    }

    curl_close($ch);
    fclose($fp);
    return TRUE;
  }

  /**
   * Upload file request
   *
   * @param $data
   *   The input data to give to the url.
   * @param $URL
   *   The path to perform the DELETE request.
   * @param $files
   *   The file(s) to post to the specified URL
   *
   * @return
   *   curl server response
   *
   *
   */
  public function uploadFile($URL, $data, $files){
    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, $URL);
    curl_setopt($ch, CURLOPT_POST, 1);
    curl_setopt($ch, CURLOPT_SAFE_UPLOAD, 1);

    if ($data !== NULL) {
      curl_setopt($ch, CURLOPT_POSTFIELDS, $data);
    }

    $i = 0;
    foreach ($files as $cfile){
      $data[array_keys($files)[$i]] = curl_file_create($cfile);
      $i++;
    }

    curl_setopt($ch, CURLOPT_POSTFIELDS, $data);

    curl_setopt($ch, CURLOPT_RETURNTRANSFER, True);

    $response = $this->getCURLResponse($ch);

    return $response;
  }

  /**
   * Universal DELETE request
   *
   * @param $input
   *   The input data to give to the url.
   * @param $URL
   *   The path to perform the DELETE request.
   *
   * @return
   *   curl server response
   *
   *
   */
  public function httpDELETE($URL, $input = NULL) {
    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, $URL);
    curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, True);

    if ($input != NULL) {
      curl_setopt($ch, CURLOPT_POSTFIELDS, $input);
    }

    $response = $this->getCURLResponse($ch);
    curl_close($ch);

    return $response;
  }

  /**
   *
   * @param unknown $URL
   * @param unknown $username
   * @param unknown $password
   */
  public function auth($URL, $username, $password) {
    // Instantiate the curl object.
    $ch = curl_init();

    // Set the curl options for this API call.
    curl_setopt($ch, CURLOPT_URL, $URL);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, TRUE);
    curl_setopt($ch, CURLOPT_USERPWD, $username . ':' . $password);

    // Execute the connection.
    $response = $this->getCURLResponse($ch);
    curl_close($ch);

    return $response;
  }

  /**
   *
   * @return
   *   string error message from the server or CURL.
   *
   *
   */
  public function getError() {
    return $this->error->getError();
  }

  /**
   *
   * @return The
   */
  public function getErrorMessage() {
    return $this->error->getErrorMessage();
  }
  /**
   *
   * @return The
   */
  public function getErrorType() {
    return $this->error->getErrorType();
  }

  /**
   *
   * @param $type
   * @param $message
   */
  public function setError($type, $message) {
    $this->error->setError($type, $message);
  }

  /**
   * Checks if the provided value is an array and sets an error if not.
   *
   * This is a helper function to help children class deal with the case
   * when Galaxy does not return a JSON array as expected. Sometimes when
   * there is an error the error message is returned in a JSON array
   * and can be handled by our getCURLResponse function.  But otherwise that
   * function can't distinguish between a string that is returned and an
   * error.  So, this function allows children class to check that
   * the response is an array if they expect that it should be.
   *
   * @param $response
   */
  public function expectArray($response) {

    if ($response !== FALSE and !is_array($response)) {
      $this->setError('Galaxy', 'Response is not an array: "' . print_r($response, TRUE) . '"');
      return FALSE;
    }
    return $response;
  }
}