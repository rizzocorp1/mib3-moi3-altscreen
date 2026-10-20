#!/usr/bin/env python3
"""servidor_ao_vivo.py — servidor local do visualizador da HMI do MOI3 (Marco 5, 11/10/2026).

Faz duas coisas, só para este computador (escuta em 127.0.0.1):

1. Serve a pasta ao-vivo/ (a página e os quadros que o glhost grava).
2. Recebe a entrada que a página manda (POST /entrada) e a acrescenta, linha a linha, em
   ao-vivo/entrada/comandos.txt. O dsistub, dentro do container, acompanha esse arquivo e
   publica cada linha no barramento como tecla ou toque (formato no cabeçalho do dsistub.c).

Só passam linhas no formato exato de um comando, e só de pedidos feitos pela própria página
(cabeçalho X-Moi3-Entrada, que outro site não consegue mandar sem ser barrado pelo navegador).

uso:  python3 -I servidor_ao_vivo.py <porta> <pasta ao-vivo>
O -I importa: a pasta servida recebe arquivos escritos pelo container; rodar isolado impede
que o Python carregue módulo de lá.
"""
import functools
import http.server
import os
import re
import sys

LINHA = re.compile(r"^((tecla|encoder)( -?\d{1,6}){3}|gesto( -?\d{1,6}){3,6})$")
MAIOR_CORPO = 4096


class Tratador(http.server.SimpleHTTPRequestHandler):
    arquivo_de_comandos = None

    def do_POST(self):
        if self.path != "/entrada":
            return self.send_error(404)
        origem = self.headers.get("Origin")
        if self.headers.get("X-Moi3-Entrada") != "1" or (
            origem and origem.split("://", 1)[-1] != self.headers.get("Host")
        ):
            return self.send_error(403)
        try:
            tamanho = int(self.headers.get("Content-Length", "0"))
        except ValueError:
            return self.send_error(400)
        if tamanho > MAIOR_CORPO:
            return self.send_error(413)
        corpo = self.rfile.read(tamanho).decode("ascii", "replace")
        linhas = [linha.strip() for linha in corpo.split("\n") if linha.strip()]
        if not linhas or not all(LINHA.match(linha) for linha in linhas):
            return self.send_error(400)
        with open(self.arquivo_de_comandos, "a", encoding="ascii") as f:
            f.write("".join(linha + "\n" for linha in linhas))
        self.send_response(204)
        self.end_headers()

    def end_headers(self):
        # o navegador sempre confere se a página e os quadros mudaram, em vez de usar a cópia dele
        self.send_header("Cache-Control", "no-cache")
        super().end_headers()

    def log_message(self, formato, *args):
        pass  # a página pergunta pelo quadro 12 vezes por segundo; registrar cada pedido só faria ruído


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    porta, pasta = int(sys.argv[1]), os.path.abspath(sys.argv[2])
    os.makedirs(os.path.join(pasta, "entrada"), exist_ok=True)
    Tratador.arquivo_de_comandos = os.path.join(pasta, "entrada", "comandos.txt")
    servidor = http.server.ThreadingHTTPServer(
        ("127.0.0.1", porta), functools.partial(Tratador, directory=pasta)
    )
    print(f"visualizador em http://127.0.0.1:{porta} (pasta {pasta})", flush=True)
    try:
        servidor.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
