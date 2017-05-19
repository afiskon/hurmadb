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
  curl -i -XGET localhost:8080/
  ```
  
  
## Stop

* **URL**

  /v1/_stop
  
* **Method:**

  `GET` | `PUT`
  
* **Success Response:**
  
  * **Code:** `200` <br />
 
* **Sample Call:**
  
  ```
  curl -i -XPUT localhost:8080/v1/_stop
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
  curl -i -XPUT -d 'some_value' localhost:8080/v1/kv/some_key
  
  curl -i -XGET localhost:8080/v1/kv/some_key
  
  curl -i -XDELETE localhost:8080/v1/kv/some_key
  ```
