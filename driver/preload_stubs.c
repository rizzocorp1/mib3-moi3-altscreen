/* preload_stubs.c — no-ops pra pular o registro de serviço durante a construção
 * do FctFsgSetup (não precisamos do monitor de serviços pro teste do coding).
 * Cada símbolo é um nome mangled C++ — que é identificador C válido.
 * Estudo/sandbox. */

/* Viwi::Client::ServiceListener::addServiceToMonitor(std::string const&, bool) */
long _ZN4Viwi6Client15ServiceListener19addServiceToMonitorERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEb(void* a, void* b, char c){ (void)a;(void)b;(void)c; return 0; }
