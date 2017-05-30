# REST API v1

## Get database state

  `GET /`
  
* **Successful Response:**
  
  * **Code:** `200` <br />
    **Content:** `HurmaDB is running!`
    
  OR
  
  * **Code:** `200` <br />
    **Content:** `HurmaDB is terminating!`
 
* **Example:**
  
  ```
  curl -v -XGET localhost:8080/
  ```
  
  
## Stop database

  `PUT /v1/_stop`
  
* **Successful Response:**
  
  * **Code:** `200` <br />
 
* **Example:**
  
  ```
  curl -v -XPUT localhost:8080/v1/_stop
  ```
  
  
## Store value

  `PUT /v1/kv/:key`
  
* **Data:**

  Stored value
  
* **Successful Response:**
  
  * **Code:** `200` <br />
  
* **Error Response:**

  * **Code:** `404` <br />

* **Example:**
  
  ```
  curl -v -XPUT -d 'some_value' localhost:8080/v1/kv/some_key  
  ```


## Get value

  `GET /v1/kv/:key`

* **Successful Response:**
  
  * **Code:** `200` <br /> <br />
    **Content:** `stored_value`
  
* **Error Response:**

  * **Code:** `404` <br />

* **Example:**
  
  ```  
  curl -v -XGET localhost:8080/v1/kv/some_key
  ```


## Delete value

  `GET /v1/kv/:key`

* **Successful Response:**
  
  * **Code:** `200` <br />

* **Error Response:**

  * **Code:** `404` <br />

* **Example:**
  
  ```
  curl -v -XDELETE localhost:8080/v1/kv/some_key
  ```
