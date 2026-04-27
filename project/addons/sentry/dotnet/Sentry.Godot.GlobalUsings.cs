// Create an explicit alias for "SentrySdk" that points to our wrapper.
// This avoids ambiguity when consumer code also includes "using Sentry;".
global using SentrySdk = global::Sentry.Godot.SentrySdk;
