CREATE TABLE IF NOT EXISTS users (
    id BIGSERIAL PRIMARY KEY,
    login TEXT NOT NULL UNIQUE,
    email TEXT,
    password_hash TEXT NOT NULL,
    salt TEXT NOT NULL,
    is_admin BOOLEAN DEFAULT FALSE,
    role TEXT DEFAULT 'viewer'
);

CREATE TABLE IF NOT EXISTS threats (
    threat_code TEXT PRIMARY KEY,
    title TEXT,
    description TEXT,
    consequences TEXT,
    source TEXT,
    created_by BIGINT,
    created_at TIMESTAMP DEFAULT NOW(),
    updated_by BIGINT,
    updated_at TIMESTAMP
);

CREATE TABLE IF NOT EXISTS staging_threats (
    threat_code TEXT,
    title TEXT,
    description TEXT,
    consequences TEXT,
    source TEXT
);

CREATE TABLE IF NOT EXISTS update_log (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT,
    inserted INT DEFAULT 0,
    updated INT DEFAULT 0,
    source TEXT,
    imported_at TIMESTAMP DEFAULT NOW()
);
