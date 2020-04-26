Run Postgres docker run -p 5432:5432 --name timescaledb -e POSTGRES_PASSWORD=your_password -d timescale/timescaledb:latest-pg12
Load schema with, e.g.: psql -h localhost -U postgres -f provision.sql

