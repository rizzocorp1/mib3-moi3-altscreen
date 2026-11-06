#!/bin/sh
# =============================================================================
# fix_hw_partnumber.sh - Injetor One-Shot Auto-Desarmavel & Restaurador MIB3 LG
# Carro Alvo: VW Jetta GLI 2021 (MIB3 MOI3 LG, MU0803)
#
# FILOSOFIA DE SEGURANCA TOTAL (Padrao James11123 / One-Shot Auto-Disarm):
# 1. No GEM, prepara os arquivos e cria o ingresso de uso unico: DO_TEST_ONCE.
# 2. Quando o usuario clica em 'Resume', o GEM reinicia o sistema operacional.
# 3. No boot seguinte:
#    - carplay.sh detecta o ingresso DO_TEST_ONCE.
#    - DELETA O INGRESSO IMEDIATAMENTE (rm -f DO_TEST_ONCE).
#    - RESTAURA O carplay.sh DE FABRICA NO DISCO IMEDIATAMENTE.
#    - Ativa o teste (bind-mount) exclusivamente para esta sessao em andamento.
# 4. Se houver crash, reinicio ou desligamento do carro:
#    - No proximo boot o ingresso ja nao existe e o script ja e o de fabrica.
#    - A central liga 100% ORIGINAL DE FABRICA. Risco de loop: ZERO.
# =============================================================================

# 1. Localiza a raiz do pendrive USB
USB_DIR=""
for p in /tmp/USB/PORT1/PART1 /tmp/USB/PORT2/PART1 /media/usb0 /media/usb1 /mnt/usb; do
    if [ -f "$p/fix_hw_partnumber.sh" ]; then
        USB_DIR="$p"
        break
    fi
done

# Coleta imediatamente logs de sessoes anteriores caso existam
if [ -n "$USB_DIR" ]; then
    # Logs especificos do handler
    [ -f /data/lge/smartlink/carplay/carplay-handler.log ] && cp -f /data/lge/smartlink/carplay/carplay-handler.log "$USB_DIR/carplay_handler.log" 2>/dev/null || true
    [ -f /data/lge/smartlink/carplay/carplay_live_journal.log ] && cp -f /data/lge/smartlink/carplay/carplay_live_journal.log "$USB_DIR/carplay_live_journal.log" 2>/dev/null || true
    [ -f /data/lge/smartlink/carplay/carplay_stdout.log ] && cp -f /data/lge/smartlink/carplay/carplay_stdout.log "$USB_DIR/carplay_stdout.log" 2>/dev/null || true
    [ -f /lge/app_rw/smartlink/carplay/carplay-handler.log ] && cp -f /lge/app_rw/smartlink/carplay/carplay-handler.log "$USB_DIR/carplay_handler_apprw.log" 2>/dev/null || true
    [ -f /tmp/carplay-handler.log ] && cp -f /tmp/carplay-handler.log "$USB_DIR/carplay_handler_tmp.log" 2>/dev/null || true

    # Diagnostico completo do sistema (journal do carplay, kernel dmesg e syslog)
    if [ -x /bin/journalctl ] || [ -x /usr/bin/journalctl ]; then
        journalctl -u carplay.service -b -1 --no-pager > "$USB_DIR/carplay_journal_prev.log" 2>/dev/null || true
        journalctl -u carplay.service --no-pager -n 300 > "$USB_DIR/carplay_journal_curr.log" 2>/dev/null || true
    fi
    dmesg > "$USB_DIR/dmesg.log" 2>/dev/null || true
    dmesg | grep -i ALTSCREEN > "$USB_DIR/altscreen_kmsg.log" 2>/dev/null || true
    if [ -d /sys/fs/pstore ]; then
        mkdir -p "$USB_DIR/pstore_logs" 2>/dev/null || true
        cp -rf /sys/fs/pstore/* "$USB_DIR/pstore_logs/" 2>/dev/null || true
    fi
    [ -f /var/log/messages ] && tail -n 500 /var/log/messages > "$USB_DIR/messages.log" 2>/dev/null || true
    sync 2>/dev/null || true
fi

CARPLAY_DIR="/lge/app_rw/smartlink/carplay"
ORIG_SH="$CARPLAY_DIR/carplay.sh"
ORIG_BIN="$CARPLAY_DIR/carplay"
ORIG_MDNSD="$CARPLAY_DIR/mdnsd"
ORIG_IAP2D="$CARPLAY_DIR/iAP2d"

FACTORY_SH_BACKUP="$CARPLAY_DIR/carplay_factory.sh"
TEST_BIN="$CARPLAY_DIR/carplay_altscreen"
ONESHOT_FLAG="$CARPLAY_DIR/DO_TEST_ONCE"

# =============================================================================
# MODO 1: ATIVACAO ONE-SHOT ALTSCREEN v4.5 (LD_PRELOAD CLEAN INTERPOSITION)
# =============================================================================
if [ -n "$USB_DIR" ] && [ -f "$USB_DIR/ENABLE_ALTSCREEN.txt" ] && [ ! -f "$USB_DIR/REVERT.txt" ]; then
    LOG_FILE="$USB_DIR/deploy_success.log"
    echo "=================================================" > "$LOG_FILE"
    echo "DEPLOY ALTSCREEN v6.2 (GENUINE ALTSCREEN GEN-2 CLUSTER STREAMER) - $(date)" >> "$LOG_FILE"
    echo "USB Mount: $USB_DIR" >> "$LOG_FILE"
    echo "=================================================" >> "$LOG_FILE"

    echo "[+] Verificando payload..." >> "$LOG_FILE"
    if [ ! -f "$USB_DIR/payload/altscreen_handler.so" ] || [ ! -f "$USB_DIR/payload/carplay_launcher" ] || [ ! -f "$USB_DIR/factory/carplay" ]; then
        echo "[ERRO] Arquivos de payload ou factory/carplay ausentes no pendrive!" >> "$LOG_FILE"
        exit 0
    fi

    # 1.1 Garante que temos a copia do carplay.sh original de fabrica guardada
    echo "[+] Criando backup de seguranca do carplay.sh de fabrica..." >> "$LOG_FILE"
    cat << 'EOF' > "$FACTORY_SH_BACKUP"
chmod 0777 /sys/devices/platform/extcon-iap2.0/iap2_mode 2>/dev/null || true
chmod 0777 /sys/devices/platform/extcon-iap2.0/iap2_mode2 2>/dev/null || true
chmod 0777 /sys/devices/platform/extcon-iap2.0/iap2_state 2>/dev/null || true
exit 0
EOF
    chmod 755 "$FACTORY_SH_BACKUP" 2>/dev/null || true
    chown 0:0 "$FACTORY_SH_BACKUP" 2>/dev/null || true
    if [ -f /usr/bin/chsmack ]; then
        /usr/bin/chsmack -a "smartlink::app" "$FACTORY_SH_BACKUP" 2>/dev/null || true
    fi

    # 1.2 Pre-cria os arquivos de log com permissao de escrita para o servico carplay
    echo "[+] Configurando permissoes dos logs persistentes..." >> "$LOG_FILE"
    mkdir -p /data/lge/smartlink/carplay 2>/dev/null || true
    > /data/lge/smartlink/carplay/carplay-handler.log 2>/dev/null || true
    > /data/lge/smartlink/carplay/carplay_stdout.log 2>/dev/null || true
    chmod 666 /data/lge/smartlink/carplay/carplay-handler.log /data/lge/smartlink/carplay/carplay_stdout.log 2>/dev/null || true
    > /lge/app_rw/smartlink/carplay/carplay-handler.log 2>/dev/null || true
    chmod 666 /lge/app_rw/smartlink/carplay/carplay-handler.log 2>/dev/null || true
    if [ -f /usr/bin/chsmack ]; then
        /usr/bin/chsmack -a "*" /data/lge/smartlink/carplay/carplay-handler.log 2>/dev/null || true
        /usr/bin/chsmack -a "*" /data/lge/smartlink/carplay/carplay_stdout.log 2>/dev/null || true
        /usr/bin/chsmack -a "*" /lge/app_rw/smartlink/carplay/carplay-handler.log 2>/dev/null || true
    fi

    # 1.3 Instala a biblioteca do hook na pasta oficial de bibliotecas da central e no diretorio local
    echo "[+] Instalando altscreen_handler.so em /lge/app_rw/lib64 e $CARPLAY_DIR..." >> "$LOG_FILE"
    mkdir -p /lge/app_rw/lib64 2>/dev/null || true
    cp -f "$USB_DIR/payload/altscreen_handler.so" /lge/app_rw/lib64/altscreen_handler.so 2>/dev/null || true
    cp -f "$USB_DIR/payload/altscreen_handler.so" "$CARPLAY_DIR/altscreen_handler.so" 2>/dev/null || true
    chmod 755 /lge/app_rw/lib64/altscreen_handler.so "$CARPLAY_DIR/altscreen_handler.so" 2>/dev/null || true
    chown 0:0 /lge/app_rw/lib64/altscreen_handler.so "$CARPLAY_DIR/altscreen_handler.so" 2>/dev/null || true
    if [ -f /usr/bin/chsmack ]; then
        /usr/bin/chsmack -a "smartlink::app" -e "smartlink::app" /lge/app_rw/lib64/altscreen_handler.so 2>/dev/null || true
        /usr/bin/chsmack -a "smartlink::app" -e "smartlink::app" "$CARPLAY_DIR/altscreen_handler.so" 2>/dev/null || true
    fi

    # 1.4 Prepara o binario original genuino (carplay_real) e o launcher ELF nativo
    echo "[+] Preparando carplay_real (genuino de fabrica) e carplay_launcher (ELF nativo)..." >> "$LOG_FILE"
    if [ -f "$USB_DIR/factory/carplay" ]; then
        cp -f "$USB_DIR/factory/carplay" "$CARPLAY_DIR/carplay_real" 2>/dev/null || true
    else
        [ -f "$CARPLAY_DIR/carplay_real" ] || cp -f "$ORIG_BIN" "$CARPLAY_DIR/carplay_real" 2>/dev/null || true
    fi
    chmod 755 "$CARPLAY_DIR/carplay_real" 2>/dev/null || true
    chown 0:0 "$CARPLAY_DIR/carplay_real" 2>/dev/null || true

    cp -f "$USB_DIR/payload/carplay_launcher" "$CARPLAY_DIR/carplay_launcher" 2>/dev/null || true
    chmod 755 "$CARPLAY_DIR/carplay_launcher" 2>/dev/null || true
    chown 0:0 "$CARPLAY_DIR/carplay_launcher" 2>/dev/null || true

    if [ -f /usr/bin/chsmack ]; then
        /usr/bin/chsmack -a "smartlink::app" -e "carplay::app" "$CARPLAY_DIR/carplay_real" 2>/dev/null || true
        /usr/bin/chsmack -a "smartlink::app" -e "carplay::app" "$CARPLAY_DIR/carplay_launcher" 2>/dev/null || true
    fi

    # 1.5 Cria o ingresso One-Shot (valido para exatamente 1 inicializacao)
    echo "[+] Criando ingresso One-Shot (DO_TEST_ONCE)..." >> "$LOG_FILE"
    touch "$ONESHOT_FLAG" 2>/dev/null || true
    chmod 644 "$ONESHOT_FLAG" 2>/dev/null || true
    chown 0:0 "$ONESHOT_FLAG" 2>/dev/null || true

    # 1.6 Configura o carplay.sh auto-desarmavel para o boot do 'Resume'
    echo "[+] Configurando carplay.sh com auto-desarme de fabrica no primeiro segundo do boot..." >> "$LOG_FILE"
    cat << 'EOF' > "$ORIG_SH"
#!/bin/sh
# Permissoes padrao de fabrica para comunicacao USB / iAP2
chmod 0777 /sys/devices/platform/extcon-iap2.0/iap2_mode 2>/dev/null || true
chmod 0777 /sys/devices/platform/extcon-iap2.0/iap2_mode2 2>/dev/null || true
chmod 0777 /sys/devices/platform/extcon-iap2.0/iap2_state 2>/dev/null || true

FLAG="/lge/app_rw/smartlink/carplay/DO_TEST_ONCE"
LAUNCHER="/lge/app_rw/smartlink/carplay/carplay_launcher"
REAL_BIN="/lge/app_rw/smartlink/carplay/carplay_real"
TARGET="/lge/app_rw/smartlink/carplay/carplay"
FACTORY_SH="/lge/app_rw/smartlink/carplay/carplay_factory.sh"
CURRENT_SH="/lge/app_rw/smartlink/carplay/carplay.sh"

# Se o ingresso de teste existir:
if [ -f "$FLAG" ] && [ -f "$LAUNCHER" ] && [ -f "$REAL_BIN" ]; then
    # 1. CONSUME O INGRESSO IMEDIATAMENTE (apaga do disco)
    rm -f "$FLAG" 2>/dev/null || true

    # 2. RESTAURA O CARPLAY.SH ORIGINAL DE FABRICA NO DISCO IMEDIATAMENTE
    # Desta forma, se houver reboot por qualquer motivo, o proximo boot ja e 100% de fabrica!
    if [ -f "$FACTORY_SH" ]; then
        cp -f "$FACTORY_SH" "$CURRENT_SH" 2>/dev/null || true
    fi

    # 3. Aplica o launcher ELF nativo exclusivamente para esta sessao em andamento
    mount --bind "$LAUNCHER" "$TARGET" 2>/dev/null || true

    # 4. Inicia captura continua do journal do carplay para /data
    (
        if [ -x /bin/journalctl ] || [ -x /usr/bin/journalctl ]; then
            journalctl -f -u carplay.service > /data/lge/smartlink/carplay/carplay_live_journal.log 2>&1
        fi
    ) >/dev/null 2>&1 &

    # 5. Logger daemon em background: sincroniza logs no pendrive continuamente se inserido
    (
        mkdir -p /tmp/usbmnt 2>/dev/null || true
        while true; do
            sleep 3
            for dev in /dev/sd[a-z][0-9] /dev/sd[a-z]; do
                if [ -b "$dev" ]; then
                    mount -o rw "$dev" /tmp/usbmnt 2>/dev/null || mount "$dev" /tmp/usbmnt 2>/dev/null || true
                fi
            done
            for u in /tmp/usbmnt /tmp/USB/PORT1/PART1 /tmp/USB/PORT2/PART1 /media/usb0 /media/usb1 /mnt/usb; do
                if [ -d "$u" ] && [ -f "$u/ENABLE_ALTSCREEN.txt" ]; then
                    mount -o remount,rw "$u" 2>/dev/null || true
                    [ -f /data/lge/smartlink/carplay/carplay-handler.log ] && cp -f /data/lge/smartlink/carplay/carplay-handler.log "$u/carplay_handler.log" 2>/dev/null
                    [ -f /data/lge/smartlink/carplay/carplay_live_journal.log ] && cp -f /data/lge/smartlink/carplay/carplay_live_journal.log "$u/carplay_live_journal.log" 2>/dev/null
                    [ -f /data/lge/smartlink/carplay/carplay_stdout.log ] && cp -f /data/lge/smartlink/carplay/carplay_stdout.log "$u/carplay_stdout.log" 2>/dev/null
                    [ -f /lge/app_rw/smartlink/carplay/carplay-handler.log ] && cp -f /lge/app_rw/smartlink/carplay/carplay-handler.log "$u/carplay_handler_apprw.log" 2>/dev/null
                    [ -f /tmp/carplay-handler.log ] && cp -f /tmp/carplay-handler.log "$u/carplay_handler_tmp.log" 2>/dev/null
                    [ -f /data/lge/smartlink/carplay/server_info_v62.plist ] && cp -f /data/lge/smartlink/carplay/server_info_v62.plist "$u/server_info_v62.plist" 2>/dev/null
                    dmesg | grep -i ALTSCREEN > "$u/altscreen_kmsg.log" 2>/dev/null
                    sync 2>/dev/null
                fi
            done
        done
    ) >/dev/null 2>&1 &
fi

exit 0
EOF
    chmod 755 "$ORIG_SH" 2>/dev/null || true
    chown 0:0 "$ORIG_SH" 2>/dev/null || true
    if [ -f /usr/bin/chsmack ]; then
        /usr/bin/chsmack -a "smartlink::app" "$ORIG_SH" 2>/dev/null || true
    fi

    echo "=================================================" >> "$LOG_FILE"
    echo "STATUS: ONE-SHOT ARMADO COM SUCESSO!" >> "$LOG_FILE"
    echo "Ao clicar em 'Resume' no GEM, a central vai reiniciar." >> "$LOG_FILE"
    echo "No boot, o teste rodara 1 vez e ja deixara os arquivos de fabrica restaurados." >> "$LOG_FILE"
    echo "Em caso de falha ou reboot subsequente: volta 100% original sem risco." >> "$LOG_FILE"
    echo "=================================================" >> "$LOG_FILE"
    /bin/sync
    /bin/sync
    exit 0
fi

# =============================================================================
# MODO 2: RESTAURACAO TOTAL DE FABRICA (REVERT)
# =============================================================================
LOG_FILE="/tmp/restore_carplay.log"
if [ -n "$USB_DIR" ]; then
    LOG_FILE="$USB_DIR/restore_success.log"
fi

echo "=================================================" > "$LOG_FILE"
echo "RESTAURACAO TOTAL DE FABRICA DO CARPLAY - $(date)" >> "$LOG_FILE"
echo "USB Mount: $USB_DIR" >> "$LOG_FILE"
echo "=================================================" >> "$LOG_FILE"

# 2.1 Desmonta qualquer bind mount ativo
umount -f /etc/vw_fs_reboot.sh 2>/dev/null || true
umount -f "$CARPLAY_DIR/carplay" 2>/dev/null || true
umount -f "$CARPLAY_DIR/carplay_patched" 2>/dev/null || true

# 2.2 Apaga ingresso e patches
rm -f "$ONESHOT_FLAG" "$TEST_BIN" "$FACTORY_SH_BACKUP" \
      "$CARPLAY_DIR/altscreen_handler.so" /lge/app_rw/lib64/altscreen_handler.so \
      /tmp/carplay_altscreen /tmp/carplay-handler.log /tmp/carplay-streaming.active 2>/dev/null || true

# 2.3 Restaura os binarios genuinos de fabrica do pendrive
if [ -d "$USB_DIR/factory" ]; then
    echo "[+] Restaurando binarios genuinos de fabrica do pendrive..." >> "$LOG_FILE"
    [ -f "$USB_DIR/factory/carplay" ] && cp -f "$USB_DIR/factory/carplay" "$ORIG_BIN"
    [ -f "$USB_DIR/factory/mdnsd" ] && cp -f "$USB_DIR/factory/mdnsd" "$ORIG_MDNSD"
    [ -f "$USB_DIR/factory/iAP2d" ] && cp -f "$USB_DIR/factory/iAP2d" "$ORIG_IAP2D"
    [ -f "$USB_DIR/factory/carplay.sh" ] && cp -f "$USB_DIR/factory/carplay.sh" "$ORIG_SH"
else
    cat << 'EOF' > "$ORIG_SH"
chmod 0777 /sys/devices/platform/extcon-iap2.0/iap2_mode 2>/dev/null || true
chmod 0777 /sys/devices/platform/extcon-iap2.0/iap2_mode2 2>/dev/null || true
chmod 0777 /sys/devices/platform/extcon-iap2.0/iap2_state 2>/dev/null || true
exit 0
EOF
fi

# 2.4 Corrige permissoes e propriedades de dono
chown 0:0 "$CARPLAY_DIR"/* 2>/dev/null || true
chmod 755 "$CARPLAY_DIR"/* 2>/dev/null || true

# 2.5 Aplica os rotulos SMACK exatos de fabrica
if [ -f /usr/bin/chsmack ]; then
    /usr/bin/chsmack -a "smartlink::app" "$ORIG_SH" 2>/dev/null || true
    /usr/bin/chsmack -a "smartlink::app" -e "carplay::app" "$ORIG_BIN" 2>/dev/null || true
    /usr/bin/chsmack -a "smartlink::app" -e "smartlink::app" "$ORIG_MDNSD" 2>/dev/null || true
    /usr/bin/chsmack -a "smartlink::app" -e "smartlink::app" "$ORIG_IAP2D" 2>/dev/null || true
    [ -d "$CARPLAY_DIR/res" ] && /usr/bin/chsmack -a "smartlink::app" "$CARPLAY_DIR/res" 2>/dev/null || true
    [ -d "$CARPLAY_DIR/res" ] && /usr/bin/chsmack -t "$CARPLAY_DIR/res" 2>/dev/null || true
fi

if [ -f /etc/smack/conf/carplay.sh ]; then
    /bin/sh /etc/smack/conf/carplay.sh >> "$LOG_FILE" 2>&1 || true
fi

chmod 0777 /sys/devices/platform/extcon-iap2.0/iap2_mode 2>/dev/null || true
chmod 0777 /sys/devices/platform/extcon-iap2.0/iap2_mode2 2>/dev/null || true
chmod 0777 /sys/devices/platform/extcon-iap2.0/iap2_state 2>/dev/null || true

mkdir -p /data/lge/smartlink/carplay 2>/dev/null || true
chown -R sint_sec_carplay_display:video /data/lge/smartlink 2>/dev/null || true
chmod -R 755 /data/lge/smartlink 2>/dev/null || true
if [ -f /usr/bin/chsmack ]; then
    /usr/bin/chsmack -rt /data/lge/smartlink/carplay 2>/dev/null || true
    /usr/bin/chsmack -ra "carplay::data" /data/lge/smartlink/carplay 2>/dev/null || true
fi

# 2.6 Reinicia os servicos systemd de forma limpa e sequencial
echo "[+] Reiniciando servicos systemd do CarPlay..." >> "$LOG_FILE"
systemctl reset-failed carplay.service carplay-iap2d.service carplay-bonjour.service 2>/dev/null || true
systemctl stop carplay.service 2>/dev/null || true
systemctl stop carplay-iap2d.service 2>/dev/null || true
systemctl stop carplay-bonjour.service 2>/dev/null || true
systemctl daemon-reload 2>/dev/null || true
systemctl start carplay-bonjour.service 2>/dev/null || true
systemctl start carplay-iap2d.service 2>/dev/null || true
systemctl start carplay.service 2>/dev/null || true

sleep 2
echo "STATUS: RESTAURACAO DE FABRICA CONCLUIDA COM SUCESSO!" >> "$LOG_FILE"
echo "=================================================" >> "$LOG_FILE"
/bin/sync
/bin/sync
exit 0
