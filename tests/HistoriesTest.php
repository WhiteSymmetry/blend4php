<?php
require_once '../src/Histories.inc';
require_once '../src/GalaxyInstance.inc';
require_once 'testConfig.inc';

class HistoriesTest extends PHPUnit_Framework_TestCase {

  /**
   *
   */
  public function testCreate() {

    global $config;

    $galaxy = new GalaxyInstance($config['host'], $config['port']);
    $galaxy->authenticate($config['user'], $config['pass']);
    $hist = new Histories($galaxy);

//    $hist->create('testhistorycreate');

    $response = $hist->GET($config['host'] . ':' . $config['port'] . '/api/histories/?key=' . $config['api_key']);

    $i = 0;
    while (array_key_exists('name', $response[$i])) {

      if ("testhistorycreate" == $response[$i]['name']) {
        break;
      }
      $i++ ;
    }

    $this->assertEquals('testhistorycreate', $response[$i]['name']);
  }

  /**
   *
   * @return Json
   */
  public function testIndex() {

    global $config;

    $galaxy = new GalaxyInstance($config['host'], $config['port']);
    $galaxy->authenticate($config['user'], $config['pass']);
    $hist = new Histories($galaxy);

    $response = $hist->index();

    // Now we check again to make sure the response is valid and we can
    // find 'testhistorycreate'
    $i = 0;
    while (array_key_exists('name', $response[$i])) {

      if ("testhistorycreate" == $response[$i]['name']) {
        break;
      }
      $i++ ;
    }

    $this->assertEquals('testhistorycreate', $response[$i]['name']);

    return $response[$i];
  }

  /**
   * @depends testIndex
   *
   * @param json $response
   */
  public function testShow($response) {

    global $config;

    $galaxy = new GalaxyInstance($config['host'], $config['port']);
    $galaxy->authenticate($config['user'], $config['pass']);
    $hist = new Histories($galaxy);

    $result = $hist->show($response['id']);

    $this->assertEquals('testhistorycreate', $result['name']);

    return $result;
  }

  /**
   * @depends testShow
   */
  public function testArchiveDownload($result) {

    global $config;

    $galaxy = new GalaxyInstance($config['host'], $config['port']);
    $galaxy->authenticate($config['user'], $config['pass']);
    $hist = new Histories($galaxy);

    $response = $hist->archiveDownload($result['id']);

  }
}
