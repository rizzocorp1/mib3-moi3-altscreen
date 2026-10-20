// janela.swift — a central MOI3 do sandbox numa janela do Mac.
//
// É estudo num computador: a HMI do firmware roda no container da bancada e nada vai para o carro.
// A janela recebe a tela pela transmissão direta do glhost (formato no glforward-moi3/host.c) e
// devolve toque, teclas (as da moldura e as do volante) e botões giratórios como linhas de comando
// (formato no driver/dsistub.c).
// O container roda sem rede; o que atravessa é a entrada e a saída padrão de
//     docker exec -i <container> <ponte> <soquete dos quadros> <arquivo de comandos>
// (glforward-moi3/ponte.c). Compilar: sh build.sh, nesta pasta. Só usa o que já vem no macOS.
//
// Uso:
//   central-moi3 [opções]                       abre a janela
//   central-moi3 --decodifica <bruto> <ppm>     aplica um fluxo gravado e grava a imagem final
//   central-moi3 --captura <ppm> [segundos]     liga na central, lista os quadros e grava o último
// Opções: --container <nome>  --ponte <caminho>  --soquete <caminho>  --entrada <caminho>
//         --docker <caminho>  --pasta <mib3-sandbox>  --segundos <n>  (duração ao ligar a central)
//         --manda "<linha>;<linha>"   (com --captura: comandos mandados depois do primeiro quadro)
//         --roteiro "<passo>;<passo>" (a janela se usa sozinha; passos no fim deste arquivo)

import AppKit

// MARK: - Quadros (formato no glforward-moi3/host.c)

@inline(__always) func le16(_ b: UnsafeBufferPointer<UInt8>, _ i: Int) -> UInt32 {
    UInt32(b[i]) | UInt32(b[i + 1]) << 8
}
@inline(__always) func le32(_ b: UnsafeBufferPointer<UInt8>, _ i: Int) -> UInt32 {
    UInt32(b[i]) | UInt32(b[i + 1]) << 8 | UInt32(b[i + 2]) << 16 | UInt32(b[i + 3]) << 24
}

struct Cabecalho {
    static let tamanho = 20
    let seq: UInt32
    let largura: Int
    let altura: Int
    let chave: Bool
    let bytes: Int
}

/// A imagem da tela, atualizada pelos quadros que chegam. Quatro bytes por pixel (RGB e um de sobra).
final class Imagem {
    private(set) var largura = 0
    private(set) var altura = 0
    private(set) var pixels = [UInt8]()

    /// Aplica a carga de um quadro. Devolve false se ela não cabe na imagem.
    func aplica(_ c: Cabecalho, _ carga: UnsafeBufferPointer<UInt8>) -> Bool {
        if c.chave || c.largura != largura || c.altura != altura {
            guard c.chave else { return false }
            largura = c.largura
            altura = c.altura
            pixels = [UInt8](repeating: 0, count: largura * altura * 4)
        }
        let n = largura * altura
        return pixels.withUnsafeMutableBufferPointer { dst -> Bool in
            var pos = 0, p = 0
            let fim = carga.count
            while p < fim {
                guard fim - p >= 8 else { return false }
                let pula = Int(le32(carga, p)), copia = Int(le32(carga, p + 4))
                p += 8
                guard pos + pula + copia <= n, copia * 3 <= fim - p else { return false }
                pos += pula
                var d = pos * 4
                for _ in 0..<copia {
                    dst[d] = carga[p]
                    dst[d + 1] = carga[p + 1]
                    dst[d + 2] = carga[p + 2]
                    d += 4
                    p += 3
                }
                pos += copia
            }
            return true
        }
    }

    func cgImage() -> CGImage? {
        guard largura > 0, let dados = CGDataProvider(data: Data(pixels) as CFData),
              let espaco = CGColorSpace(name: CGColorSpace.sRGB) else { return nil }
        return CGImage(width: largura, height: altura, bitsPerComponent: 8, bitsPerPixel: 32,
                       bytesPerRow: largura * 4, space: espaco,
                       bitmapInfo: CGBitmapInfo(rawValue: CGImageAlphaInfo.noneSkipLast.rawValue),
                       provider: dados, decode: nil, shouldInterpolate: true, intent: .defaultIntent)
    }

    func ppm() -> Data {
        var d = Data("P6\n\(largura) \(altura)\n255\n".utf8)
        var rgb = [UInt8](repeating: 0, count: largura * altura * 3)
        for i in 0..<(largura * altura) {
            rgb[3 * i] = pixels[4 * i]
            rgb[3 * i + 1] = pixels[4 * i + 1]
            rgb[3 * i + 2] = pixels[4 * i + 2]
        }
        d.append(contentsOf: rgb)
        return d
    }
}

/// Junta os pedaços do fluxo e entrega cada mensagem completa.
struct LeitorDeQuadros {
    private var guardado = [UInt8]()

    /// Devolve false se o que chegou não é a transmissão dos quadros.
    mutating func recebe(_ pedaco: UnsafeBufferPointer<UInt8>,
                         _ quadro: (Cabecalho, UnsafeBufferPointer<UInt8>) -> Bool) -> Bool {
        guardado.append(contentsOf: pedaco)
        var usado = 0, certo = true
        guardado.withUnsafeBufferPointer { b in
            while b.count - usado >= Cabecalho.tamanho {
                let i = usado
                guard b[i] == 0x4D, b[i + 1] == 0x51, b[i + 2] == 0x30, b[i + 3] == 0x31 else { certo = false; return }   // "MQ01"
                let c = Cabecalho(seq: le32(b, i + 4), largura: Int(le16(b, i + 8)), altura: Int(le16(b, i + 10)),
                                  chave: le16(b, i + 12) & 1 == 1, bytes: Int(le32(b, i + 16)))
                guard c.largura > 0, c.altura > 0, c.bytes <= c.largura * c.altura * 5 + 16 else { certo = false; return }
                guard b.count - i >= Cabecalho.tamanho + c.bytes else { return }
                let carga = UnsafeBufferPointer(rebasing: b[(i + Cabecalho.tamanho)..<(i + Cabecalho.tamanho + c.bytes)])
                guard quadro(c, carga) else { certo = false; return }
                usado += Cabecalho.tamanho + c.bytes
            }
        }
        if usado > 0 { guardado.removeFirst(usado) }
        return certo
    }
}

/// Lê o descritor até o fim, entregando cada pedaço; para se o bloco devolver false.
func leFluxo(_ fd: Int32, _ aoPedaco: (UnsafeBufferPointer<UInt8>) -> Bool) {
    let cap = 1 << 20
    let buf = UnsafeMutablePointer<UInt8>.allocate(capacity: cap)
    defer { buf.deallocate() }
    while true {
        let r = read(fd, buf, cap)
        if r < 0 && errno == EINTR { continue }
        if r <= 0 { return }
        if !aoPedaco(UnsafeBufferPointer(start: buf, count: r)) { return }
    }
}

func diz(_ s: String) {
    print(s)
    fflush(stdout)
}

// MARK: - Onde está cada coisa

struct Config {
    var docker = ""
    var container = "moi3-ao-vivo"
    var ponte = "/tmp/w/glfwd/ponte"               // onde o driver/m3_ao_vivo.sh compila o encaminhador
    var soquete = "/tmp/moi3-quadros.sock"         // default do glhost
    var entrada = "/mnt/low/tmp/dsi-entrada"       // o arquivo de comandos que o dsistub acompanha
    var pasta: String?                             // a pasta mib3-sandbox, para ligar a central daqui
    var segundos = 7200
    var roteiro: String?
    var manda: String?

    var comandoDaPonte: [String] { ["exec", "-i", container, ponte, soquete, entrada] }
}

func achaDocker(_ pedido: String?) -> String {
    var onde = [String]()
    if let p = pedido { onde.append(p) }
    let amb = ProcessInfo.processInfo.environment
    if let p = amb["MOI3_DOCKER"] { onde.append(p) }
    for d in (amb["PATH"] ?? "").split(separator: ":") { onde.append("\(d)/docker") }
    onde += ["/usr/local/bin/docker", "/opt/homebrew/bin/docker", NSHomeDirectory() + "/.orbstack/bin/docker",
             "/Applications/OrbStack.app/Contents/MacOS/xbin/docker",
             "/Applications/Docker.app/Contents/Resources/bin/docker"]
    return onde.first { FileManager.default.isExecutableFile(atPath: $0) } ?? ""
}

/// Sobe das pastas do programa até achar a bancada (a que tem driver/ao_vivo.sh).
func achaPasta(_ pedido: String?) -> String? {
    let fm = FileManager.default
    if let p = pedido ?? ProcessInfo.processInfo.environment["MOI3_SANDBOX"] {
        return fm.fileExists(atPath: p + "/driver/ao_vivo.sh") ? p : nil
    }
    var u = Bundle.main.bundleURL.resolvingSymlinksInPath()
    for _ in 0..<6 {
        if fm.fileExists(atPath: u.appendingPathComponent("driver/ao_vivo.sh").path) { return u.path }
        u = u.deletingLastPathComponent()
    }
    return nil
}

// MARK: - A ligação com a central

/// Roda a ponte dentro do container, monta a imagem numa thread e entrega o resultado na principal.
final class Ligacao {
    let cfg: Config
    var aoQuadro: ((CGImage, Cabecalho, Int) -> Void)?     // imagem, último cabeçalho, quadros desde a última entrega
    var aoCair: ((Int32, String) -> Void)?                 // código de saída e o que o docker disse
    private var processo: Process?
    private var cano: FileHandle?
    private let trava = NSLock()
    private var espera: (CGImage, Cabecalho, Int)?

    init(_ cfg: Config) { self.cfg = cfg }

    func inicia() {
        let p = Process(), entrada = Pipe(), saida = Pipe(), erro = Pipe()
        p.executableURL = URL(fileURLWithPath: cfg.docker)
        p.arguments = cfg.comandoDaPonte
        p.standardInput = entrada
        p.standardOutput = saida
        p.standardError = erro
        do { try p.run() } catch {
            let dito = error.localizedDescription
            DispatchQueue.main.async { [weak self] in self?.aoCair?(-1, dito) }
            return
        }
        processo = p
        cano = entrada.fileHandleForWriting
        let fd = saida.fileHandleForReading.fileDescriptor
        Thread.detachNewThread { [weak self] in
            var leitor = LeitorDeQuadros()
            let imagem = Imagem()
            leFluxo(fd) { pedaco in
                var ultimo: Cabecalho?
                var quantos = 0
                let certo = leitor.recebe(pedaco) { c, carga in
                    guard imagem.aplica(c, carga) else { return false }
                    ultimo = c
                    quantos += 1
                    return true
                }
                if let c = ultimo, let cg = imagem.cgImage() { self?.publica(cg, c, quantos) }
                return certo
            }
            if p.isRunning { p.terminate() }           // parou de ler antes do fim: o fluxo veio errado
            let dito = String(data: erro.fileHandleForReading.readDataToEndOfFile(), encoding: .utf8) ?? ""
            p.waitUntilExit()
            let codigo = p.terminationStatus
            withExtendedLifetime(saida) { }
            DispatchQueue.main.async {
                guard let eu = self else { return }
                if eu.processo === p { eu.processo = nil; eu.cano = nil }
                eu.aoCair?(codigo, dito.trimmingCharacters(in: .whitespacesAndNewlines))
            }
        }
    }

    /// Se a janela demora a desenhar, os quadros se juntam: só o último vai para a tela.
    private func publica(_ cg: CGImage, _ c: Cabecalho, _ quantos: Int) {
        trava.lock()
        let jaAvisou = espera != nil
        espera = (cg, c, quantos + (espera?.2 ?? 0))
        trava.unlock()
        if jaAvisou { return }
        DispatchQueue.main.async { [weak self] in
            guard let eu = self else { return }
            eu.trava.lock()
            let e = eu.espera
            eu.espera = nil
            eu.trava.unlock()
            if let e = e { eu.aoQuadro?(e.0, e.1, e.2) }
        }
    }

    /// Manda uma linha de comando para a central. Devolve false se não há ligação.
    func manda(_ linha: String) -> Bool {
        guard let c = cano, processo?.isRunning == true else { return false }
        do { try c.write(contentsOf: Data((linha + "\n").utf8)); return true } catch { return false }
    }

    func para() {
        try? cano?.close()
        processo?.terminate()
    }
}

// MARK: - Cores e letras (as mesmas da página ao-vivo/index.html)

func cor(_ hex: UInt32) -> NSColor {
    NSColor(srgbRed: CGFloat((hex >> 16) & 255) / 255, green: CGFloat((hex >> 8) & 255) / 255,
            blue: CGFloat(hex & 255) / 255, alpha: 1)
}

enum Cor {
    static let painel = cor(0x20262b), vidro = cor(0x050608), texto = cor(0xe8ebed), apagado = cor(0x8b959c)
    static let verde = cor(0x5fd08a), ambar = cor(0xf0b23c), vermelho = cor(0xe0655c)
    static let tecla = cor(0xb9c2c8), sobre = cor(0x12161a), apertada = cor(0x242c32)
}

func letra(_ tamanho: CGFloat, forte: Bool = false) -> NSFont {
    NSFont(name: forte ? "AvenirNext-DemiBold" : "AvenirNext-Regular", size: tamanho)
        ?? NSFont.systemFont(ofSize: tamanho, weight: forte ? .semibold : .regular)
}

// MARK: - A tela

/// Mostra a imagem da central e transforma o mouse em toque.
final class TelaView: NSView {
    var imagem: CGImage? { didSet { needsDisplay = true } }
    var aoGesto: ((String, String) -> Void)?           // linha de comando e o que dizer no rodapé

    private let solta = 3, encosta = 4, arrastaG = 5, toque = 1    // gestos que a HMI entende
    private let folga = 8          // o dedo treme: antes disso não é arrasto
    private let limite = 25        // além disso a HMI não aceita como clique (CLICK_THRESHOLD dela)
    private var dedo: (x0: Int, y0: Int, x: Int, y: Int, arrastou: Bool, longe: Bool, ultimo: TimeInterval)?

    override init(frame: NSRect) {
        super.init(frame: frame)
        wantsLayer = true
        setAccessibilityElement(true)
        setAccessibilityRole(.image)
        setAccessibilityLabel("Tela da central MOI3")
    }
    required init?(coder: NSCoder) { fatalError("sem xib") }

    override var wantsUpdateLayer: Bool { true }
    override func updateLayer() {
        guard let l = layer else { return }
        l.backgroundColor = NSColor.black.cgColor
        l.cornerRadius = 3
        l.masksToBounds = true
        l.contentsGravity = .resize
        l.contents = imagem
        // em escala inteira, cada pixel da central vira um quadrado nítido; fora disso, suaviza
        if let i = imagem {
            let escala = bounds.width * (window?.backingScaleFactor ?? 2) / CGFloat(i.width)
            l.magnificationFilter = abs(escala - escala.rounded()) < 0.01 ? .nearest : .linear
            l.minificationFilter = .trilinear
        }
    }
    override func draw(_ dirtyRect: NSRect) {          // só para quem pede a imagem da janela (foto)
        NSColor.black.setFill()
        bounds.fill()
        if let i = imagem { NSGraphicsContext.current?.cgContext.draw(i, in: bounds) }
    }
    override func setFrameSize(_ s: NSSize) {
        super.setFrameSize(s)
        needsDisplay = true
    }
    override func acceptsFirstMouse(for event: NSEvent?) -> Bool { true }

    /// Do ponto da janela para o pixel da central (y contado de cima).
    private func ponto(_ ev: NSEvent) -> (Int, Int)? {
        guard let i = imagem, bounds.width > 0, bounds.height > 0 else { return nil }
        let p = convert(ev.locationInWindow, from: nil)
        let x = Int((p.x * CGFloat(i.width) / bounds.width).rounded())
        let y = Int(((bounds.height - p.y) * CGFloat(i.height) / bounds.height).rounded())
        return (min(max(x, 0), i.width - 1), min(max(y, 0), i.height - 1))
    }
    /// O ponto da janela que corresponde a um pixel da central (para o roteiro).
    func lugar(_ x: Int, _ y: Int) -> NSPoint {
        let w = CGFloat(imagem?.width ?? 800), h = CGFloat(imagem?.height ?? 480)
        return NSPoint(x: CGFloat(x) * bounds.width / w, y: bounds.height - CGFloat(y) * bounds.height / h)
    }

    override func mouseDown(with ev: NSEvent) {
        guard dedo == nil, let (x, y) = ponto(ev) else { return }
        dedo = (x, y, x, y, false, false, ProcessInfo.processInfo.systemUptime)
        aoGesto?("gesto \(encosta) \(x) \(y)", "toque em \(x), \(y)")
    }
    override func mouseDragged(with ev: NSEvent) {
        guard var d = dedo, let (x, y) = ponto(ev) else { return }
        let agora = ProcessInfo.processInfo.systemUptime
        let anda = max(abs(x - d.x0), abs(y - d.y0))
        if anda > folga { d.arrastou = true }
        if anda > limite { d.longe = true }
        dedo = d
        if !d.arrastou || (x == d.x && y == d.y) || agora - d.ultimo < 0.04 { return }
        d.x = x; d.y = y; d.ultimo = agora
        dedo = d
        aoGesto?("gesto \(arrastaG) \(x) \(y)", "arrastando em \(x), \(y)")
    }
    override func mouseUp(with ev: NSEvent) {
        guard let d = dedo else { return }
        dedo = nil
        let (x, y) = ponto(ev) ?? (d.x, d.y)
        // parado, o gesto termina como "toque" (a HMI trata como clique); se o dedo andou, só solta
        if d.longe { aoGesto?("gesto \(solta) \(x) \(y)", "arrasto até \(x), \(y)") }
        else { aoGesto?("gesto \(toque) \(x) \(y) 1", "toque em \(x), \(y)") }
    }
}

// MARK: - As teclas e os botões giratórios

/// Uma tecla da moldura: fica apertada enquanto o mouse estiver apertado, para a HMI ver o toque longo.
final class TeclaView: NSView {
    let nome: String
    var dica: String?
    var aoMudar: ((Bool) -> Void)?             // true ao apertar, false ao soltar
    var fonte = letra(13) { didSet { needsDisplay = true } }
    private(set) var apertada = false { didSet { needsDisplay = true } }
    private var sobre = false { didSet { needsDisplay = true } }
    private var area: NSTrackingArea?

    init(_ nome: String, dica: String? = nil) {
        self.nome = nome
        self.dica = dica
        super.init(frame: .zero)
        toolTip = dica
    }
    required init?(coder: NSCoder) { fatalError("sem xib") }

    var larguraDoTexto: CGFloat { ceil((nome as NSString).size(withAttributes: [.font: fonte, .kern: 0.4]).width) }

    func aperta() { if !apertada { apertada = true; aoMudar?(true) } }
    func larga() { if apertada { apertada = false; aoMudar?(false) } }

    override func acceptsFirstMouse(for event: NSEvent?) -> Bool { true }
    override func updateTrackingAreas() {
        super.updateTrackingAreas()
        if let a = area { removeTrackingArea(a) }
        let a = NSTrackingArea(rect: bounds, options: [.mouseEnteredAndExited, .activeInKeyWindow, .inVisibleRect],
                               owner: self, userInfo: nil)
        addTrackingArea(a)
        area = a
    }
    override func mouseEntered(with event: NSEvent) { sobre = true }
    override func mouseExited(with event: NSEvent) { sobre = false }
    override func mouseDown(with event: NSEvent) { aperta() }
    override func mouseUp(with event: NSEvent) { larga() }

    override func draw(_ dirtyRect: NSRect) {
        if apertada || sobre {
            (apertada ? Cor.apertada : Cor.sobre).setFill()
            NSBezierPath(roundedRect: bounds, xRadius: 5, yRadius: 5).fill()
        }
        let atributos: [NSAttributedString.Key: Any] = [
            .font: fonte, .kern: 0.4, .foregroundColor: (apertada || sobre) ? NSColor.white : Cor.tecla]
        let t = nome as NSString, s = t.size(withAttributes: atributos)
        t.draw(at: NSPoint(x: ((bounds.width - s.width) / 2).rounded(), y: ((bounds.height - s.height) / 2).rounded()),
               withAttributes: atributos)
    }

    override func isAccessibilityElement() -> Bool { true }
    override func accessibilityRole() -> NSAccessibility.Role? { .button }
    override func accessibilityLabel() -> String? { dica ?? nome }
    override func accessibilityPerformPress() -> Bool {
        aperta()
        larga()
        return true
    }
}

/// Um botão giratório: rolar em cima gira, clicar aperta.
final class GiratorioView: NSView {
    let nome: String
    let sentido: Int                           // no volume, rolar para cima aumenta; na seleção, rolar para baixo avança
    var aoGirar: ((Int) -> Void)?
    var aoMudar: ((Bool) -> Void)?
    private var angulo: CGFloat = 0
    private var resto: CGFloat = 0
    private var apertada = false { didSet { needsDisplay = true } }
    private var sobre = false { didSet { needsDisplay = true } }
    private var area: NSTrackingArea?

    init(_ nome: String, sentido: Int, dica: String) {
        self.nome = nome
        self.sentido = sentido
        super.init(frame: .zero)
        toolTip = dica
    }
    required init?(coder: NSCoder) { fatalError("sem xib") }

    func gira(_ passos: Int) {
        guard passos != 0 else { return }
        angulo += CGFloat(passos) * 18
        needsDisplay = true
        aoGirar?(passos)
    }

    override func acceptsFirstMouse(for event: NSEvent?) -> Bool { true }
    override func updateTrackingAreas() {
        super.updateTrackingAreas()
        if let a = area { removeTrackingArea(a) }
        let a = NSTrackingArea(rect: bounds, options: [.mouseEnteredAndExited, .activeInKeyWindow, .inVisibleRect],
                               owner: self, userInfo: nil)
        addTrackingArea(a)
        area = a
    }
    override func mouseEntered(with event: NSEvent) { sobre = true }
    override func mouseExited(with event: NSEvent) { sobre = false }
    override func mouseDown(with event: NSEvent) { if !apertada { apertada = true; aoMudar?(true) } }
    override func mouseUp(with event: NSEvent) { if apertada { apertada = false; aoMudar?(false) } }
    override func scrollWheel(with ev: NSEvent) {
        // o AppKit dá valor positivo ao rolar para cima; a conta é a da página, que usa o sinal contrário.
        // Trackpad e mouse com rolagem fina vêm em pontos (40 por passo); roda comum, um passo por dente.
        resto += ev.hasPreciseScrollingDeltas ? -ev.scrollingDeltaY : -ev.scrollingDeltaY * 40
        let passos = Int((resto / 40).rounded(.towardZero))
        resto -= CGFloat(passos) * 40
        gira(passos * sentido)
    }

    override func draw(_ dirtyRect: NSRect) {
        let r = bounds.insetBy(dx: 1, dy: 1)
        let disco = NSBezierPath(ovalIn: r)
        NSGradient(starting: cor(sobre ? 0x3d474e : 0x333c43), ending: cor(sobre ? 0x1d2327 : 0x181d21))?
            .draw(in: disco, relativeCenterPosition: NSPoint(x: 0, y: 0.24))
        cor(apertada ? 0x6b7780 : 0x3d474e).setStroke()
        disco.lineWidth = 1
        disco.stroke()
        // a marca que mostra quanto girou: no alto do disco, girando no sentido do relógio
        let gira = NSAffineTransform()
        gira.translateX(by: bounds.midX, yBy: bounds.midY)
        gira.rotate(byDegrees: -angulo)
        let marca = NSBezierPath(roundedRect: NSRect(x: -1.5, y: bounds.height / 2 - 16, width: 3, height: 11),
                                 xRadius: 1.5, yRadius: 1.5)
        marca.transform(using: gira as AffineTransform)
        cor(0xcfd6da).setFill()
        marca.fill()
    }

    override func isAccessibilityElement() -> Bool { true }
    override func accessibilityRole() -> NSAccessibility.Role? { .button }
    override func accessibilityLabel() -> String? { toolTip }
    override func accessibilityPerformPress() -> Bool {
        aoMudar?(true)
        aoMudar?(false)
        return true
    }
}

final class LuzView: NSView {
    var acesa = Cor.vermelho { didSet { needsDisplay = true } }
    override func draw(_ dirtyRect: NSRect) {
        acesa.setFill()
        NSBezierPath(ovalIn: bounds).fill()
    }
}

// MARK: - O painel: a tela, a moldura com as teclas e a linha de estado

final class Painel: NSView {
    let tela = TelaView(frame: .zero)
    let aviso = NSTextField(wrappingLabelWithString: "")
    let botaoLigar = NSButton(title: "Ligar a central", target: nil, action: nil)
    let volume = GiratorioView("volume", sentido: -1, dica: "Volume: role para girar, clique para apertar (teclas − e =)")
    let selecao = GiratorioView("seleção", sentido: 1, dica: "Seleção: role para girar, clique para escolher (setas e Return)")
    let volumeMenos = TeclaView("−", dica: "Diminuir o volume")
    let volumeMais = TeclaView("+", dica: "Aumentar o volume")
    let selecaoMenos = TeclaView("−", dica: "Item anterior")
    let selecaoMais = TeclaView("+", dica: "Próximo item")
    // códigos de org.dsi.ifc.keypanel.Constants; só o que a HMI atende nesta bancada
    let teclas: [(TeclaView, Int)] = [("Menu", 78), ("Rádio", 15), ("Mídia", 1), ("Telefone", 3), ("Carro", 6),
                                      ("Clima", 79), ("Som", 86), ("Navegação", 4), ("Ajustes", 7),
                                      ("App", 114)]   // KEY_SMARTPHONE: a tecla APP da moldura do Jetta (foto de 09/10/2026); sem atalho de número
        .enumerated().map { (TeclaView($0.element.0, dica: "\($0.element.0) (tecla \($0.offset + 1))"), $0.element.1) }
    // o volante multifunção (teclado 4, KBD_MFW): as onze teclas que a tabela de teclas da HMI tem para ele.
    // Rótulo, nome no roteiro, código (os KEY_MFW_* das mesmas constantes) e dica.
    let volante: [(tecla: TeclaView, nome: String, codigo: Int)] = [
        ("Vol −", "vol-", 43, "diminuir o volume"), ("Vol +", "vol+", 42, "aumentar o volume"),
        ("Rolete", "rolete", 44, "rolete da direita"), ("Mudo", "mudo", 57, "mudo"),
        ("Anterior", "anterior", 47, "faixa ou emissora anterior"), ("Próxima", "proxima", 46, "próxima faixa ou emissora"),
        ("▼", "baixo", 49, "seta para baixo"), ("▲", "cima", 48, "seta para cima"),
        ("Fonte", "fonte", 45, "fonte de áudio"), ("Voz", "voz", 50, "comando de voz"), ("Telefone", "telefone", 53, "telefone"),
    ].map { (TeclaView($0.0, dica: "Volante: \($0.3)"), $0.1, $0.2) }
    let rotuloVolante = NSTextField(labelWithString: "Volante")
    let luz = LuzView(frame: .zero)
    let estado = NSTextField(labelWithString: "Procurando a central…")
    let leitura = NSTextField(labelWithString: "")
    let entrada = NSTextField(labelWithString: "")
    var proporcao: CGFloat = 800.0 / 480.0 { didSet { needsLayout = true } }
    var aoTeclar: ((NSEvent, Bool) -> Bool)?       // tecla do teclado do Mac; devolve true se usou
    private var aparelho = NSRect.zero
    private var faixaDoVolante = NSRect.zero
    static let vaoDoVolante: CGFloat = 10          // entre o aparelho e a faixa do volante
    static let alturaDoVolante = vaoDoVolante + 46 // o que a fileira do volante pede de altura: o vão e a faixa

    override init(frame: NSRect) {
        super.init(frame: frame)
        addSubview(tela)
        aviso.font = letra(15)
        aviso.textColor = Cor.apagado
        aviso.alignment = .center
        aviso.maximumNumberOfLines = 4
        addSubview(aviso)
        botaoLigar.bezelStyle = .rounded
        botaoLigar.isHidden = true
        addSubview(botaoLigar)
        for v in [volumeMenos, volumeMais, selecaoMenos, selecaoMais] { v.fonte = letra(16); addSubview(v) }
        addSubview(volume)
        addSubview(selecao)
        for (t, _) in teclas { addSubview(t) }
        rotuloVolante.font = letra(13)
        rotuloVolante.textColor = Cor.apagado
        addSubview(rotuloVolante)
        for v in volante { addSubview(v.tecla) }
        addSubview(luz)
        estado.font = letra(14, forte: true)
        estado.textColor = Cor.texto
        leitura.font = NSFont.monospacedDigitSystemFont(ofSize: 13, weight: .regular)
        leitura.textColor = Cor.apagado
        leitura.alignment = .right
        entrada.font = letra(13)
        entrada.textColor = Cor.apagado
        entrada.lineBreakMode = .byTruncatingTail
        for l in [estado, leitura, entrada] { addSubview(l) }
    }
    required init?(coder: NSCoder) { fatalError("sem xib") }

    override var isFlipped: Bool { true }
    override var acceptsFirstResponder: Bool { true }
    override func keyDown(with ev: NSEvent) { if aoTeclar?(ev, true) != true { super.keyDown(with: ev) } }
    override func keyUp(with ev: NSEvent) { if aoTeclar?(ev, false) != true { super.keyUp(with: ev) } }

    override func layout() {
        super.layout()
        let margem: CGFloat = 20, borda: CGFloat = 16, linha: CGFloat = 46, vao: CGFloat = 12, rodape: CGFloat = 60
        // a tela ocupa o que sobra, sem deformar
        let cabeL = bounds.width - 2 * (margem + borda)
        let cabeA = bounds.height - 2 * margem - 2 * borda - vao - linha - Painel.alturaDoVolante - rodape
        let l = max(240, min(cabeL, cabeA * proporcao)).rounded(.down), a = (l / proporcao).rounded()
        let x0 = ((bounds.width - l) / 2).rounded()
        aparelho = NSRect(x: x0 - borda, y: margem, width: l + 2 * borda, height: a + 2 * borda + vao + linha)
        tela.frame = NSRect(x: x0, y: margem + borda, width: l, height: a)
        aviso.frame = NSRect(x: x0 + 40, y: tela.frame.midY - 44, width: l - 80, height: 66)
        botaoLigar.sizeToFit()
        botaoLigar.frame.origin = NSPoint(x: (tela.frame.midX - botaoLigar.frame.width / 2).rounded(), y: tela.frame.midY + 30)

        // a moldura: botão de volume, as teclas e o botão de seleção, numa linha só
        let y = tela.frame.maxY + vao, passo: CGFloat = 22, grupo = passo + 2 + linha + 2 + passo
        func poe(_ menos: TeclaView, _ botao: GiratorioView, _ mais: TeclaView, _ x: CGFloat) {
            menos.frame = NSRect(x: x, y: y + 11, width: passo, height: 24)
            botao.frame = NSRect(x: x + passo + 2, y: y, width: linha, height: linha)
            mais.frame = NSRect(x: x + passo + 2 + linha + 2, y: y + 11, width: passo, height: 24)
        }
        poe(volumeMenos, volume, volumeMais, x0)
        poe(selecaoMenos, selecao, selecaoMais, x0 + l - grupo)
        /// Uma fileira de teclas, centrada no espaço que cabe a ela.
        func fileira(_ ts: [TeclaView], de xi: CGFloat, cabe: CGFloat, y: CGFloat) {
            let entre: CGFloat = 2
            var tamanho: CGFloat = 13, texto: CGFloat = 0, folga: CGFloat = 0
            repeat {                               // janela estreita: a letra encolhe antes de as teclas se atropelarem
                for t in ts { t.fonte = letra(tamanho) }
                texto = ts.reduce(0) { $0 + $1.larguraDoTexto }
                folga = (cabe - texto - entre * CGFloat(ts.count - 1)) / CGFloat(2 * ts.count)
                tamanho -= 1
            } while folga < 3 && tamanho >= 10
            folga = min(max(folga, 2), 11)
            var x = xi + ((cabe - texto - 2 * folga * CGFloat(ts.count) - entre * CGFloat(ts.count - 1)) / 2).rounded()
            for t in ts {
                let w = t.larguraDoTexto + 2 * folga
                t.frame = NSRect(x: x.rounded(), y: y, width: w.rounded(), height: 30)
                x += w + entre
            }
        }
        fileira(teclas.map { $0.0 }, de: x0 + grupo + 10, cabe: l - 2 * grupo - 20, y: y + 8)

        // o volante: uma faixa à parte, embaixo do aparelho, com o nome e as teclas dele
        faixaDoVolante = NSRect(x: aparelho.minX, y: aparelho.maxY + Painel.vaoDoVolante, width: aparelho.width,
                                height: Painel.alturaDoVolante - Painel.vaoDoVolante)
        rotuloVolante.sizeToFit()
        let rotulo = rotuloVolante.frame.size
        rotuloVolante.frame.origin = NSPoint(x: x0, y: (faixaDoVolante.midY - rotulo.height / 2).rounded())
        fileira(volante.map { $0.tecla }, de: x0 + rotulo.width + 12, cabe: l - rotulo.width - 12, y: faixaDoVolante.minY + 8)

        // a linha de estado, embaixo do volante
        let yr = faixaDoVolante.maxY + 14
        luz.frame = NSRect(x: aparelho.minX + 6, y: yr + 6, width: 10, height: 10)
        leitura.sizeToFit()
        let lw = min(leitura.frame.width + 4, aparelho.width / 2)
        leitura.frame = NSRect(x: aparelho.maxX - 6 - lw, y: yr + 2, width: lw, height: 18)
        estado.frame = NSRect(x: luz.frame.maxX + 10, y: yr, width: leitura.frame.minX - luz.frame.maxX - 20, height: 22)
        entrada.frame = NSRect(x: aparelho.minX + 6, y: yr + 26, width: aparelho.width - 12, height: 18)
    }

    override func draw(_ dirtyRect: NSRect) {
        Cor.painel.setFill()
        bounds.fill()
        // o aparelho e a faixa do volante: vidro preto, com a sombra embaixo e um fio de luz em cima
        let sombra = NSShadow()
        sombra.shadowColor = NSColor.black.withAlphaComponent(0.55)
        sombra.shadowBlurRadius = 24
        sombra.shadowOffset = NSSize(width: 0, height: -14)
        for vidro in [aparelho, faixaDoVolante] {
            NSGraphicsContext.saveGraphicsState()
            sombra.set()
            Cor.vidro.setFill()
            NSBezierPath(roundedRect: vidro, xRadius: 14, yRadius: 14).fill()
            NSGraphicsContext.restoreGraphicsState()
            NSColor.white.withAlphaComponent(0.08).setStroke()
            let fio = NSBezierPath()
            fio.move(to: NSPoint(x: vidro.minX + 14, y: vidro.minY + 0.5))
            fio.line(to: NSPoint(x: vidro.maxX - 14, y: vidro.minY + 0.5))
            fio.lineWidth = 1
            fio.stroke()
        }
    }
}

// MARK: - O controle da janela

enum Situacao {
    case procurando                // a ponte acabou de ser chamada
    case ligando                   // a central está no ar mas ainda não desenhou
    case aoVivo
    case parada(String, podeLigar: Bool)   // sem ligação: o que dizer e se o botão de ligar resolve
}

final class Controle: NSObject, NSApplicationDelegate, NSWindowDelegate {
    var cfg: Config
    var janela: NSWindow!
    let painel = Painel(frame: NSRect(x: 0, y: 0, width: 872, height: 670 + Painel.alturaDoVolante))
    var ligacao: Ligacao?
    var situacao = Situacao.procurando
    var chamadaEm: TimeInterval = 0            // quando a ponte foi chamada
    var ligandoAte: TimeInterval = 0           // depois de "Ligar a central": o container ainda vai nascer
    var quadroEm: TimeInterval = 0             // quando chegou o último quadro
    var numero: UInt32 = 0
    var tamanho = (0, 0)
    var recentes = [(TimeInterval, UInt32)]()
    var ultimaImagem: CGImage?
    var falhou = false
    var ultimaEntrada = ""
    var passosPendentes = [Int: Int]()         // tecla do botão giratório -> passos ainda não mandados
    var passos = [String]()                    // roteiro
    var quadrosRecebidos = 0                   // quadros que a ponte entregou
    var entregas = 0                           // vezes em que a janela trocou a imagem
    let teclado = 13                           // KBD_TOUCHSCREEN_FRONT: a tela e as teclas ao redor dela
    let tecladoDoVolante = 4                   // KBD_MFW: o volante multifunção
    let numeros: NumberFormatter = {
        let f = NumberFormatter()
        f.locale = Locale(identifier: "pt_BR")
        f.numberStyle = .decimal
        f.maximumFractionDigits = 1
        return f
    }()

    init(_ cfg: Config) { self.cfg = cfg }

    var agora: TimeInterval { ProcessInfo.processInfo.systemUptime }

    // MARK: montagem

    func applicationDidFinishLaunching(_ note: Notification) {
        janela = NSWindow(contentRect: painel.frame, styleMask: [.titled, .closable, .miniaturizable, .resizable],
                          backing: .buffered, defer: false)
        janela.title = "Central MOI3"
        janela.appearance = NSAppearance(named: .darkAqua)
        janela.titlebarAppearsTransparent = true
        janela.backgroundColor = Cor.painel
        janela.contentView = painel
        janela.contentMinSize = NSSize(width: 700, height: 500 + Painel.alturaDoVolante)
        janela.delegate = self
        janela.center()
        // A fileira do volante (12/10/2026) pede mais altura. Quem já tinha a janela guardada ganha essa
        // altura uma vez, com o alto da janela onde estava, para a tela continuar do tamanho que tinha.
        let guardados = UserDefaults.standard
        let tinhaJanela = guardados.string(forKey: "NSWindow Frame central-moi3") != nil
        janela.setFrameAutosaveName("central-moi3")
        if !guardados.bool(forKey: "cresceu-para-o-volante") {
            guardados.set(true, forKey: "cresceu-para-o-volante")
            if tinhaJanela {
                var f = janela.frame
                f.origin.y -= Painel.alturaDoVolante
                f.size.height += Painel.alturaDoVolante
                if let cabe = janela.screen?.visibleFrame, f.minY < cabe.minY, f.height <= cabe.height { f.origin.y = cabe.minY }
                janela.setFrame(f, display: false)
            }
        }
        montaMenu()
        ligaControles()
        janela.makeFirstResponder(painel)
        if cfg.roteiro == nil {
            janela.makeKeyAndOrderFront(nil)
            NSApp.activate(ignoringOtherApps: true)
        } else {
            janela.orderFront(nil)                 // sozinha, a janela não rouba o teclado de ninguém
            passos = cfg.roteiro!.split(separator: ";").map { $0.trimmingCharacters(in: .whitespaces) }
            diz("janela \(janela.windowNumber)")
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) { [weak self] in self?.proximoPasso() }
        }
        chama()
        Timer.scheduledTimer(withTimeInterval: 0.25, repeats: true) { [weak self] _ in self?.atualiza() }
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool { true }
    func applicationWillTerminate(_ note: Notification) { ligacao?.para() }

    func montaMenu() {
        let barra = NSMenu()
        let app = NSMenuItem(), menuApp = NSMenu()
        menuApp.addItem(withTitle: "Ocultar a Central MOI3", action: #selector(NSApplication.hide(_:)), keyEquivalent: "h")
        menuApp.addItem(.separator())
        menuApp.addItem(withTitle: "Encerrar a Central MOI3", action: #selector(NSApplication.terminate(_:)), keyEquivalent: "q")
        app.submenu = menuApp
        barra.addItem(app)
        let central = NSMenuItem(), menuCentral = NSMenu(title: "Central")
        menuCentral.addItem(withTitle: "Ligar a central", action: #selector(ligaCentral), keyEquivalent: "l").target = self
        menuCentral.addItem(withTitle: "Desligar a central", action: #selector(desligaCentral), keyEquivalent: "").target = self
        menuCentral.addItem(.separator())
        menuCentral.addItem(withTitle: "Salvar a tela em PNG…", action: #selector(salvaTela), keyEquivalent: "s").target = self
        central.submenu = menuCentral
        barra.addItem(central)
        let jan = NSMenuItem(), menuJan = NSMenu(title: "Janela")
        menuJan.addItem(withTitle: "Minimizar", action: #selector(NSWindow.performMiniaturize(_:)), keyEquivalent: "m")
        menuJan.addItem(withTitle: "Fechar", action: #selector(NSWindow.performClose(_:)), keyEquivalent: "w")
        jan.submenu = menuJan
        barra.addItem(jan)
        NSApp.mainMenu = barra
        NSApp.windowsMenu = menuJan
    }

    func ligaControles() {
        painel.tela.aoGesto = { [weak self] linha, rotulo in self?.manda(linha, rotulo) }
        for (t, codigo) in painel.teclas {
            t.aoMudar = { [weak self, weak t] apertou in
                guard let eu = self else { return }
                eu.manda("tecla \(eu.teclado) \(codigo) \(apertou ? 1 : 0)", "tecla \(t?.nome ?? "")")
            }
        }
        for (t, _, codigo) in painel.volante {
            t.aoMudar = { [weak self, weak t] apertou in
                guard let eu = self else { return }
                eu.manda("tecla \(eu.tecladoDoVolante) \(codigo) \(apertou ? 1 : 0)", "tecla \(t?.nome ?? "") do volante")
            }
        }
        // os botões giratórios: 17 é o de volume, 16 o de seleção
        for (botao, codigo, menos, mais) in [(painel.volume, 17, painel.volumeMenos, painel.volumeMais),
                                              (painel.selecao, 16, painel.selecaoMenos, painel.selecaoMais)] {
            botao.aoMudar = { [weak self, weak botao] apertou in
                guard let eu = self else { return }
                eu.manda("tecla \(eu.teclado) \(codigo) \(apertou ? 1 : 0)", "botão de \(botao?.nome ?? "") apertado")
            }
            botao.aoGirar = { [weak self, weak botao] n in self?.girou(codigo, n, botao?.nome ?? "") }
            menos.aoMudar = { [weak botao] apertou in if apertou { botao?.gira(-1) } }
            mais.aoMudar = { [weak botao] apertou in if apertou { botao?.gira(1) } }
        }
        painel.aoTeclar = { [weak self] ev, desceu in self?.teclou(ev, desceu) ?? false }
        painel.botaoLigar.target = self
        painel.botaoLigar.action = #selector(ligaCentral)
    }

    // MARK: ligação

    func chama() {
        if cfg.docker.isEmpty {
            situacao = .parada("Não achei o programa docker neste Mac. Instale o OrbStack ou indique o caminho com --docker.", podeLigar: false)
            atualiza()
            return
        }
        let l = Ligacao(cfg)
        l.aoQuadro = { [weak self] imagem, c, quantos in self?.chegou(imagem, c, quantos) }
        l.aoCair = { [weak self] codigo, dito in self?.caiu(codigo, dito) }
        ligacao = l
        chamadaEm = agora
        switch situacao {                          // a cada tentativa o aviso continua o mesmo
        case .parada, .ligando: break
        default: situacao = .procurando
        }
        l.inicia()
    }

    func chegou(_ imagem: CGImage, _ c: Cabecalho, _ quantos: Int) {
        quadrosRecebidos += quantos
        entregas += 1
        situacao = .aoVivo
        ultimaImagem = imagem
        painel.tela.imagem = imagem
        if tamanho != (c.largura, c.altura) {
            tamanho = (c.largura, c.altura)
            painel.proporcao = CGFloat(c.largura) / CGFloat(c.altura)
        }
        if c.seq < numero { recentes = [] }
        numero = c.seq
        quadroEm = agora
        recentes.append((quadroEm, c.seq))
    }

    func caiu(_ codigo: Int32, _ dito: String) {
        ligacao = nil
        ultimaImagem = nil
        painel.tela.imagem = nil
        recentes = []
        ultimaEntrada = ""
        falhou = false
        let d = dito.lowercased()
        if agora < ligandoAte {
            situacao = .ligando
        } else if d.contains("no such container") || d.contains("is not running") {
            situacao = .parada("A central está desligada.", podeLigar: cfg.pasta != nil)
        } else if d.contains("cannot connect") || d.contains("docker daemon") || d.contains("connection refused") {
            situacao = .parada("O Docker não respondeu. Abra o OrbStack; a janela tenta de novo sozinha.", podeLigar: false)
        } else if codigo == 3 {
            situacao = .parada("A central no ar foi ligada antes de existir a transmissão direta. Desligue e ligue de novo pelo menu Central.", podeLigar: false)
        } else if codigo == 126 || codigo == 127 || d.contains("no such file") || d.contains("not found") {
            situacao = .ligando                    // o container subiu e ainda está compilando a bancada
        } else if codigo == -1 {
            situacao = .parada("Não consegui chamar o docker: \(dito)", podeLigar: false)
        } else {
            situacao = .parada("A central foi desligada.", podeLigar: cfg.pasta != nil)
        }
        atualiza()
        DispatchQueue.main.asyncAfter(deadline: .now() + 1.5) { [weak self] in
            if self?.ligacao == nil { self?.chama() }
        }
    }

    func manda(_ linha: String, _ rotulo: String) {
        if cfg.roteiro != nil { diz("manda: \(linha)") }
        falhou = !(ligacao?.manda(linha) ?? false)
        ultimaEntrada = falhou ? "A entrada não chegou à central: ela não está ligada." : "Último: \(rotulo)."
    }

    /// Junta os passos de uma rolada rápida num evento só.
    func girou(_ codigo: Int, _ n: Int, _ nome: String) {
        let ja = passosPendentes[codigo] != nil
        passosPendentes[codigo, default: 0] += n
        if ja { return }
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.06) { [weak self] in
            guard let eu = self, let total = eu.passosPendentes.removeValue(forKey: codigo), total != 0 else { return }
            eu.manda("encoder \(eu.teclado) \(codigo) \(total)", "botão de \(nome) \(total > 0 ? "+" : "−")\(abs(total))")
        }
    }

    /// O teclado do Mac: 1 a 9 são as teclas da moldura, as setas giram a seleção, Return aperta,
    /// − e = giram o volume.
    func teclou(_ ev: NSEvent, _ desceu: Bool) -> Bool {
        if !ev.modifierFlags.intersection([.command, .control, .option]).isEmpty { return false }
        switch ev.keyCode {
        case 125, 124: if desceu { painel.selecao.gira(1) }; return true       // seta para baixo, para a direita
        case 126, 123: if desceu { painel.selecao.gira(-1) }; return true      // seta para cima, para a esquerda
        case 36, 76:                                                           // Return, Enter
            if !ev.isARepeat { manda("tecla \(teclado) 16 \(desceu ? 1 : 0)", "botão de seleção apertado") }
            return true
        default: break
        }
        guard let c = ev.charactersIgnoringModifiers, c.count == 1 else { return false }
        if c == "-" || c == "=" || c == "+" {
            if desceu { painel.volume.gira(c == "-" ? -1 : 1) }
            return true
        }
        if let n = Int(c), n >= 1, n <= painel.teclas.count {
            if !ev.isARepeat { let t = painel.teclas[n - 1].0; if desceu { t.aperta() } else { t.larga() } }
            return true
        }
        return false
    }

    // MARK: o que a janela diz

    func atualiza() {
        let t = agora
        recentes.removeAll { t - $0.0 > 3 }
        var texto = "", luz = Cor.vermelho, aviso = "", ligar = false
        var leitura = ""
        if ligacao != nil, t - chamadaEm > 2 {     // a ponte está de pé e ainda não veio quadro
            switch situacao {
            case .aoVivo, .ligando: break
            default: situacao = .ligando
            }
        }
        switch situacao {
        case .procurando:
            texto = "Procurando a central…"
            aviso = "Procurando a central…"
        case .ligando:
            texto = "A central está ligando"
            luz = Cor.ambar
            aviso = "A central está ligando.\nA primeira tela leva cerca de um minuto."
        case .aoVivo:
            let parado = Int(t - quadroEm)
            if parado < 3 { texto = "Ao vivo"; luz = Cor.verde }
            else { texto = "Sem quadro novo há \(parado) s"; luz = Cor.ambar }
            leitura = "\(tamanho.0) × \(tamanho.1)"
            if recentes.count > 1, parado < 3, let a = recentes.first, let b = recentes.last, b.0 > a.0 {
                let ritmo = Double(b.1 &- a.1) / (b.0 - a.0)
                leitura += "    \(numeros.string(from: NSNumber(value: ritmo)) ?? "") quadros/s"
            }
            leitura += "    quadro \(numeros.string(from: NSNumber(value: numero)) ?? "")"
        case .parada(let motivo, let pode):
            texto = "Sem ligação com a central"
            aviso = motivo
            ligar = pode
        }
        painel.estado.stringValue = texto
        painel.luz.acesa = luz
        painel.aviso.stringValue = aviso
        painel.aviso.isHidden = aviso.isEmpty
        painel.botaoLigar.isHidden = !ligar
        if painel.leitura.stringValue != leitura {
            painel.leitura.stringValue = leitura
            painel.needsLayout = true
        }
        let dica = "Clique na tela para tocar, arraste para rolar. Nos botões redondos, role para girar e clique para apertar."
        if !ultimaEntrada.isEmpty { painel.entrada.stringValue = ultimaEntrada }
        else if case .aoVivo = situacao { painel.entrada.stringValue = dica }
        else { painel.entrada.stringValue = "" }
        painel.entrada.textColor = falhou ? Cor.ambar : Cor.apagado
    }

    // MARK: ligar, desligar, salvar

    @objc func ligaCentral() {
        guard let pasta = cfg.pasta, !cfg.docker.isEmpty else { return }
        let p = Process()
        p.executableURL = URL(fileURLWithPath: "/bin/sh")
        p.arguments = [pasta + "/driver/ao_vivo.sh", String(cfg.segundos)]
        var amb = ProcessInfo.processInfo.environment
        amb["SEM_SERVIDOR"] = "1"                  // a página do navegador é servida à parte
        amb["PATH"] = (cfg.docker as NSString).deletingLastPathComponent + ":" + (amb["PATH"] ?? "/usr/bin:/bin")
        p.environment = amb
        p.standardInput = FileHandle.nullDevice
        let registro = pasta + "/ao-vivo/log/janela-ligou.log"
        FileManager.default.createFile(atPath: registro, contents: nil)
        let saida = FileHandle(forWritingAtPath: registro) ?? FileHandle.nullDevice
        p.standardOutput = saida
        p.standardError = saida
        do {
            try p.run()
            situacao = .ligando
            ligandoAte = agora + 40
        } catch {
            situacao = .parada("Não consegui ligar a central: \(error.localizedDescription)", podeLigar: true)
        }
        atualiza()
    }

    @objc func desligaCentral() {
        guard !cfg.docker.isEmpty else { return }
        let p = Process()
        p.executableURL = URL(fileURLWithPath: cfg.docker)
        p.arguments = ["stop", cfg.container]
        p.standardInput = FileHandle.nullDevice
        p.standardOutput = FileHandle.nullDevice
        p.standardError = FileHandle.nullDevice
        try? p.run()
        ligandoAte = 0
        falhou = false
        ultimaEntrada = "Desligando a central…"
    }

    func png(_ imagem: CGImage) -> Data? { NSBitmapImageRep(cgImage: imagem).representation(using: .png, properties: [:]) }

    @objc func salvaTela() {
        guard let imagem = ultimaImagem else { return }
        let quadro = numero
        let painelDeSalvar = NSSavePanel()
        painelDeSalvar.allowedContentTypes = [.png]
        painelDeSalvar.nameFieldStringValue = "hmi-moi3-quadro-\(quadro).png"
        painelDeSalvar.beginSheetModal(for: janela) { [weak self] resposta in
            guard resposta == .OK, let url = painelDeSalvar.url, let dados = self?.png(imagem) else { return }
            try? dados.write(to: url)
        }
    }

    func validateMenuItem(_ item: NSMenuItem) -> Bool {
        switch item.action {
        case #selector(salvaTela): return ultimaImagem != nil
        case #selector(ligaCentral): if case .parada(_, true) = situacao { return true } else { return false }
        case #selector(desligaCentral): if case .parada = situacao { return false } else { return true }
        default: return true
        }
    }

    // MARK: roteiro: a janela se usa sozinha, pelos mesmos eventos que o mouse e o teclado geram
    //   espera <s> | toque <x> <y> | arrasta <x0> <y0> <x1> <y1> | tecla <nome> | volante <nome>
    //   aperta volume|selecao | passo volume|selecao <+1|-1> | rola volume|selecao <dentes>
    //   digita <tecla> | foto <png> | camadas <png> | quadro <png> | liga | desliga | sai
    // toque e arrasta usam os pixels da central; digita aceita 1 a 9, -, =, baixo, cima, enter.
    // volante aceita vol-, vol+, rolete, mudo, anterior, proxima, baixo, cima, fonte, voz, telefone.

    func rato(_ tipo: NSEvent.EventType, _ v: NSView, _ p: NSPoint) {
        guard let ev = NSEvent.mouseEvent(with: tipo, location: v.convert(p, to: nil), modifierFlags: [],
                                          timestamp: agora, windowNumber: janela.windowNumber, context: nil,
                                          eventNumber: 0, clickCount: 1, pressure: tipo == .leftMouseUp ? 0 : 1) else { return }
        janela.sendEvent(ev)
    }

    func clica(_ v: NSView, depois: @escaping () -> Void) {
        let meio = NSPoint(x: v.bounds.midX, y: v.bounds.midY)
        rato(.leftMouseDown, v, meio)
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.12) { [weak self] in
            self?.rato(.leftMouseUp, v, meio)
            depois()
        }
    }

    func giratorio(_ nome: String) -> (GiratorioView, TeclaView, TeclaView)? {
        switch nome {
        case "volume": return (painel.volume, painel.volumeMenos, painel.volumeMais)
        case "selecao": return (painel.selecao, painel.selecaoMenos, painel.selecaoMais)
        default: return nil
        }
    }

    func proximoPasso() {
        guard !passos.isEmpty else { return }
        let passo = passos.removeFirst()
        let p = passo.split(separator: " ").map(String.init)
        diz("passo: \(passo)")
        let segue: () -> Void = { [weak self] in DispatchQueue.main.asyncAfter(deadline: .now() + 0.05) { self?.proximoPasso() } }
        let simples = { (s: String) -> String in s.folding(options: .diacriticInsensitive, locale: nil).lowercased() }
        switch p.first ?? "" {
        case "espera":
            DispatchQueue.main.asyncAfter(deadline: .now() + (Double(p.count > 1 ? p[1] : "1") ?? 1)) { [weak self] in self?.proximoPasso() }
        case "toque" where p.count == 3:
            let onde = painel.tela.lugar(Int(p[1]) ?? 0, Int(p[2]) ?? 0)
            rato(.leftMouseDown, painel.tela, onde)
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.12) { [weak self] in
                guard let eu = self else { return }
                eu.rato(.leftMouseUp, eu.painel.tela, onde)
                segue()
            }
        case "arrasta" where p.count == 5:
            let n = p[1...4].map { Int($0) ?? 0 }
            let tela = painel.tela, pedacos = 12
            rato(.leftMouseDown, tela, tela.lugar(n[0], n[1]))
            for i in 1...pedacos {
                DispatchQueue.main.asyncAfter(deadline: .now() + 0.05 * Double(i)) { [weak self] in
                    let x = n[0] + (n[2] - n[0]) * i / pedacos, y = n[1] + (n[3] - n[1]) * i / pedacos
                    self?.rato(.leftMouseDragged, tela, tela.lugar(x, y))
                }
            }
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.05 * Double(pedacos + 2)) { [weak self] in
                self?.rato(.leftMouseUp, tela, tela.lugar(n[2], n[3]))
                segue()
            }
        case "tecla" where p.count == 2:
            if let t = painel.teclas.first(where: { simples($0.0.nome) == simples(p[1]) }) { clica(t.0, depois: segue) }
            else { diz("tecla desconhecida: \(p[1])"); segue() }
        case "volante" where p.count == 2:
            if let v = painel.volante.first(where: { $0.nome == simples(p[1]) }) { clica(v.tecla, depois: segue) }
            else { diz("tecla do volante desconhecida: \(p[1])"); segue() }
        case "aperta" where p.count == 2:
            if let g = giratorio(p[1]) { clica(g.0, depois: segue) } else { segue() }
        case "passo" where p.count == 3:
            if let g = giratorio(p[1]) { clica((Int(p[2]) ?? 1) < 0 ? g.1 : g.2, depois: segue) } else { segue() }
        case "rola" where p.count == 3:
            // dentes de roda comum; positivo é rolar para cima
            if let g = giratorio(p[1]), let cg = CGEvent(scrollWheelEvent2Source: nil, units: .line, wheelCount: 1,
                                                         wheel1: Int32(p[2]) ?? 1, wheel2: 0, wheel3: 0),
               let ev = NSEvent(cgEvent: cg) { g.0.scrollWheel(with: ev) }
            segue()
        case "digita" where p.count == 2:
            let codigos: [String: (UInt16, String)] = ["baixo": (125, "\u{F701}"), "cima": (126, "\u{F700}"), "enter": (36, "\r"),
                                                       "-": (27, "-"), "=": (24, "=")]
            let (codigo, letras) = codigos[p[1]] ?? (0, p[1])
            for tipo in [NSEvent.EventType.keyDown, .keyUp] {
                if let ev = NSEvent.keyEvent(with: tipo, location: .zero, modifierFlags: [], timestamp: agora,
                                             windowNumber: janela.windowNumber, context: nil, characters: letras,
                                             charactersIgnoringModifiers: letras, isARepeat: false, keyCode: codigo) {
                    janela.sendEvent(ev)
                }
            }
            segue()
        case "foto" where p.count == 2:
            if let v = janela.contentView, let rep = v.bitmapImageRepForCachingDisplay(in: v.bounds) {
                v.cacheDisplay(in: v.bounds, to: rep)
                try? rep.representation(using: .png, properties: [:])?.write(to: URL(fileURLWithPath: p[1]))
            }
            segue()
        case "camadas" where p.count == 2:
            // o que a janela entrega para o sistema desenhar: a árvore de camadas, e não o draw(_:)
            if let v = janela.contentView, let camada = v.layer,
               let rep = NSBitmapImageRep(bitmapDataPlanes: nil, pixelsWide: Int(v.bounds.width * 2), pixelsHigh: Int(v.bounds.height * 2),
                                          bitsPerSample: 8, samplesPerPixel: 4, hasAlpha: true, isPlanar: false,
                                          colorSpaceName: .deviceRGB, bytesPerRow: 0, bitsPerPixel: 0),
               let ctx = NSGraphicsContext(bitmapImageRep: rep) {
                ctx.cgContext.scaleBy(x: 2, y: 2)
                if camada.isGeometryFlipped || v.isFlipped {
                    ctx.cgContext.translateBy(x: 0, y: v.bounds.height)
                    ctx.cgContext.scaleBy(x: 1, y: -1)
                }
                camada.render(in: ctx.cgContext)
                try? rep.representation(using: .png, properties: [:])?.write(to: URL(fileURLWithPath: p[1]))
                diz("camadas: tela com conteúdo = \(painel.tela.layer?.contents != nil), filtro = \(painel.tela.layer?.magnificationFilter.rawValue ?? "?")")
            } else { diz("camadas: a janela não tem camada") }
            segue()
        case "quadro" where p.count == 2:
            if let i = ultimaImagem { try? png(i)?.write(to: URL(fileURLWithPath: p[1])) }
            diz("quadro \(numero) \(tamanho.0)x\(tamanho.1)")
            segue()
        case "liga": ligaCentral(); segue()
        case "desliga": desligaCentral(); segue()
        case "sai":
            diz("resumo: \(quadrosRecebidos) quadros recebidos, \(entregas) trocas de imagem na janela")
            NSApp.terminate(nil)
        default: diz("passo desconhecido: \(passo)"); segue()
        }
    }
}

// MARK: - Sem janela: conferir o decodificador e medir a ligação

func decodifica(_ bruto: String, _ saida: String) -> Int32 {
    guard let dados = FileManager.default.contents(atPath: bruto) else { diz("sem arquivo \(bruto)"); return 1 }
    var leitor = LeitorDeQuadros()
    let imagem = Imagem()
    var vieram = 0
    let certo = dados.withUnsafeBytes { (b: UnsafeRawBufferPointer) -> Bool in
        leitor.recebe(b.bindMemory(to: UInt8.self)) { c, carga in
            guard imagem.aplica(c, carga) else { return false }
            vieram += 1
            diz("quadro \(c.seq) \(c.largura)x\(c.altura) chave=\(c.chave ? 1 : 0) bytes=\(c.bytes)")
            return true
        }
    }
    guard certo, vieram > 0 else { diz(certo ? "nenhum quadro" : "formato errado"); return 2 }
    do { try imagem.ppm().write(to: URL(fileURLWithPath: saida)) } catch { diz("não gravou \(saida)"); return 1 }
    return 0
}

/// Liga na central pelo mesmo caminho da janela, lista cada quadro com a hora em que chegou e
/// grava o último. Com cfg.manda, manda os comandos depois do primeiro quadro.
func captura(_ cfg: Config, _ saida: String, _ segundos: Double) -> Int32 {
    guard !cfg.docker.isEmpty else { diz("sem docker"); return 1 }
    let p = Process(), entrada = Pipe(), fluxo = Pipe()
    p.executableURL = URL(fileURLWithPath: cfg.docker)
    p.arguments = cfg.comandoDaPonte
    p.standardInput = entrada
    p.standardOutput = fluxo
    do { try p.run() } catch { diz("não chamou o docker: \(error.localizedDescription)"); return 1 }
    let inicio = ProcessInfo.processInfo.systemUptime
    var leitor = LeitorDeQuadros()
    let imagem = Imagem()
    var vieram = 0, bytes = 0, mandouEm = 0.0
    DispatchQueue.global().asyncAfter(deadline: .now() + segundos) { try? entrada.fileHandleForWriting.close() }
    leFluxo(fluxo.fileHandleForReading.fileDescriptor) { pedaco in
        leitor.recebe(pedaco) { c, carga in
            guard imagem.aplica(c, carga) else { return false }
            let t = ProcessInfo.processInfo.systemUptime - inicio
            vieram += 1
            bytes += c.bytes + Cabecalho.tamanho
            var linha = String(format: "%7.3f s  quadro %u %dx%d chave=%d bytes=%d", t, c.seq, c.largura, c.altura, c.chave ? 1 : 0, c.bytes)
            if mandouEm > 0, c.bytes > 0 { linha += String(format: "  (%.0f ms depois do comando)", (t - mandouEm) * 1000) }
            diz(linha)
            if vieram == 1, let m = cfg.manda {
                // espera a tela assentar e manda: o que vier depois é a resposta da central
                DispatchQueue.global().asyncAfter(deadline: .now() + 1.0) {
                    mandouEm = ProcessInfo.processInfo.systemUptime - inicio
                    for l in m.split(separator: ";") {
                        try? entrada.fileHandleForWriting.write(contentsOf: Data((l.trimmingCharacters(in: .whitespaces) + "\n").utf8))
                    }
                    diz(String(format: "%7.3f s  mandou: %@", mandouEm, m))
                }
            }
            return true
        }
    }
    p.waitUntilExit()
    diz("resumo: \(vieram) quadros, \(bytes) bytes, saída \(p.terminationStatus)")
    guard vieram > 0 else { return 1 }
    do { try imagem.ppm().write(to: URL(fileURLWithPath: saida)) } catch { diz("não gravou \(saida)"); return 1 }
    return 0
}

// MARK: - Entrada do programa

signal(SIGPIPE, SIG_IGN)
var cfg = Config()
var soltos = [String]()
var modo = ""
var argumentos = Array(CommandLine.arguments.dropFirst())
var pedidoDocker: String?
var pedidoPasta: String?
while !argumentos.isEmpty {
    let a = argumentos.removeFirst()
    func valor() -> String { argumentos.isEmpty ? "" : argumentos.removeFirst() }
    switch a {
    case "--decodifica", "--captura": modo = a
    case "--container": cfg.container = valor()
    case "--ponte": cfg.ponte = valor()
    case "--soquete": cfg.soquete = valor()
    case "--entrada": cfg.entrada = valor()
    case "--docker": pedidoDocker = valor()
    case "--pasta": pedidoPasta = valor()
    case "--segundos": cfg.segundos = Int(valor()) ?? cfg.segundos
    case "--roteiro": cfg.roteiro = valor()
    case "--manda": cfg.manda = valor()
    default:
        if a.hasPrefix("-psn") { break }           // o Finder passa isso a programas antigos
        soltos.append(a)
    }
}
cfg.docker = achaDocker(pedidoDocker)
cfg.pasta = achaPasta(pedidoPasta)

if modo == "--decodifica" {
    guard soltos.count == 2 else { diz("uso: central-moi3 --decodifica <bruto> <saida.ppm>"); exit(2) }
    exit(decodifica(soltos[0], soltos[1]))
}
if modo == "--captura" {
    guard soltos.count >= 1 else { diz("uso: central-moi3 --captura <saida.ppm> [segundos]"); exit(2) }
    exit(captura(cfg, soltos[0], soltos.count > 1 ? Double(soltos[1]) ?? 2 : 2))
}

let aplicativo = NSApplication.shared
let controle = Controle(cfg)
aplicativo.delegate = controle
aplicativo.setActivationPolicy(.regular)
aplicativo.run()
