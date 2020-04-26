DROP DATABASE IF EXISTS incubator;

CREATE DATABASE incubator;
\c incubator

CREATE TABLE temperature ( 
  id              serial primary key, 
  temperature numeric NOT NULL,
  mac varchar (17) NOT NULL,
  created_at timestamptz NOT NULL DEFAULT now(),
  client_time timestamp NOT NULL
);

CREATE TABLE humidity ( 
  id              serial primary key, 
  humidity numeric NOT NULL,
  mac varchar (17) NOT NULL,
  created_at timestamptz NOT NULL DEFAULT now(),
  client_time timestamp NOT NULL
);
