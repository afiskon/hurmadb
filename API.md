## Index


* **URL**

  /
  
* **Method:**

  `GET`
  
* **Success Response:**
  
  * **Code:** `200` <br />
    **Content:** `HurmaDB is running!`
    
  OR
  
  * **Code:** `200` <br />
    **Content:** `HurmaDB is terminating!`
 
* **Sample Call:**
  
  ```
  curl -v -XGET localhost:8080/
  ```
  
  
## Stop

* **URL**

  /v1/_stop
  
* **Method:**

  `PUT`
  
* **Success Response:**
  
  * **Code:** `200` <br />
 
* **Sample Call:**
  
  ```
  curl -v -XPUT localhost:8080/v1/_stop
  ```
  
  
## KV

* **URL**

  /v1/kv/key
  
* **Method:**

  `GET` | `DELETE` | `PUT`
  
* **Data Params**

  Stored value.
  
  **Optional:**
  
  some_value
  
* **Success Response:**
  
  * **Code:** `200` <br />
  
* **Error Response:**

  * **Code:** `404` <br />

* **Sample Call:**
  
  ```
  curl -v -XPUT -d 'some_value' localhost:8080/v1/kv/some_key
  
  curl -v -XGET localhost:8080/v1/kv/some_key
  
  curl -v -XDELETE localhost:8080/v1/kv/some_key
  ```
